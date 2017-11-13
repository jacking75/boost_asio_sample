# Asio와 스레드 모델

## io_service 사용 모델
io_service 사용 모델은 다음의 네가지로 분류된다.
- 단일 스레드에서 단일 io_service를 사용하다
- 복수 스레드에서 io_service를 공유하다
- 쓰레드 당 io_service를 마련하다
- 복수 스레드에서 복수의 io_service를 사용하다


## 단일 스레드 모델
아래와 같이 시작한다.
```
asio;io_service io_service{1};
io_service.run();
```
이 모델에서는 data race 등을 의식할 필요가 없으니 다른 모델보다 코드를 단순하게 할 수 있다.
또, io_service 생성자의 concurrency_hint를 1로 지정함으로써 단일 스레드에서는 불필요한 배타 처리를 줄이고 성능을 조금이라도 올릴 수 있다.

### 시간이 걸리는 처리
이 모델에서 주의할 것은 각 핸들러의 실행 시간을 짧게 하는 것이다. 무거운 처리는 워커 쓰레드를 준비하고 그곳에서 실행한다.
```
void handler()
{
    auto work = asio::io_service::work{io_service};
    // work を保持させる
    worker_thread.post([work]{
        long_running_task(); // 何か時間がかかる処理
        work.get_io_service().post(next_task);
    });

    // io_service のキューが空でも work が存在するので
    // io_service::run は終了せず次のハンドラ (next_task) を待ち続ける.
}
```


## 스레드 풀 모델
이 모델에서 사용하는 io_service는 하나이다. 이 인스턴스의 멤버 함수 io_service::run 을 스레드 풀 중의 스레드가 호출한다.
```
asio::io_service io_service{};
std::vector<std::thread> thread_pool{};
for (auto i = std::size_t{0}; i < nthreads; ++i) {
    thread_pool.emplace_back([&io_service]{
        io_service.run(); // invoke run for each thread
    });
}
```
이 모델은 Boost.Asio의 [Strand의 튜터리얼](http://www.boost.org/doc/libs/1_59_0/doc/html/boost_asio/tutorial/tuttimer5.html)에도 들어 있으므로 본 적 있는 사람이 많을 것이다.  
이 모델의 이점으로서 단일 스레드 모델 대비 핸들러의 실행 시간을 의식할 필요가 없다는 것을 들 수 있다. 핸들러의 처리가 약간 무거워도 후속 햄들러는 기다리지 않고 다른 스레드에서 실행된다.  
또, 스레드 풀의 스레드 수를 1으로 했을 경우 io_service::run가 다른 스레드에서 실행된다는 것을 제외하면 단일 스레드 모델과 완전히 일치한다. 반대로 말하면 단일 스레드 모델에서 이 모델로의 이행이 쉽다는 것이다.  
한편, 기본적으로 Asio에서 제공되는 I/O Object는 스레드 세이프 하지 않으므로 Strand 또는 Mutex를 사용하여 스레드 간 동기를 해야할 필요가 있다.


### Mutex에 의한 I/O Object의 가드
I/O Object는 asio::async_write 등의 composed operation이 존재하므로 보통 I/O Object의 사용 전후로 Mutex를 Lock/Unlock 하는 것만으로는 불충분하다.  
Mutex에서 I/O Object를 보호하는 예는 Christopher Kohlhoff씨의 발표에서 소개되고 있다(https://github.com/boostcon/2011_presentations/raw/master/mon/thinking_asynchronously.pdf의 pp.95-98).  

그의 발표에서는 std::mutex 등 재래식의 Mutex을 사용하는 사례를 소개하고 있었는데 본 기사에서는 재귀적인 Lock이 가능한 Recursive Mutex 사용을 권장한다.  
왜 Recursive Mutex가 하면 Asio의 핸들러 호출 기구의 형편상 재귀적인 Lock이 불가피한 경우가 존재하기 때문이다.
그러나 Strand를 사용하는 것이 편리하고 스레드 블록도 줄일 수 있으므로 기본적으로 I/O Object와 Mutex를 조합할 필요는 없다.


## io_service per 스레드 모델
이 모델에서는 스레드마다 한개의 io_service가 존재한다.
```
std::vector<asio::io_service> io_service_pool(nio_services);
std::vector<std::thread> threads{};

for (auto& io_service : io_service_pool) {
    threads.emplace_back[&io_service]{ // bind each io_service
        io_service.run();
    }
}
```
이 모델의 경우 각 I/O Object는 하나의 스레드에 속하게 되므로 기본적으로 Strand 등을 사용하여 동기 할 필요가 없다.  
단일 스레드 모델의 설명에서 이야기한 concurrency_hint도 적용할 수 있으며, io_service의 수(쓰레드 수)를 CPU 수에 맞춰지면 퍼포먼스적으로는 스레드 풀 모델보다 좋을 것이다.  
한편, 단일 스레드 모델과 마찬가지로 핸들러의 실행 시간에 주의할 필요가 있다.


## 스레드 풀 모델 vs io_service per 스레드 모델


### 라이브러리/프레임워크 비교
스레드 풀 모델과 io_service per 스레드 모델은 모두 다중 스레드를 조합한 모델인데 어느 쪽을 채용해야 할까?  
Boost.Asio를 사용하고 있는 라이브러리/프레임워크 구현을 좀 알아봤다.  
또한 이 글의 조사에서는 Strand를 사용하고 있다==스레드 풀에서 사용할 의사가 있는 것으로 한다.(Strand의 사용 상황에 더해서 io_service::run을 복수 스레드에서 호출하는 가를 조사하고 있다).  
그러나, Strand를 올바르게 사용하지 않을 가능성이 있으므로 이들 라이브러리를 사용할 때 각자 문서 및 구현을 참조하자.  

### cpp-netlib 동기판 서버
[cpp-netlib/cpp-netlib·GitHub](https://github.com/cpp-netlib/cpp-netlib)  
단일 io_service를 사용한다 Strand를 내부에 사용하고 있고,스레드 풀 모델에서 사용 가능하다.


### cpp-netlib 비동기판 서버
I/O에 단일 io_service, 핸들러 실행에 다른 io_service를 스레드 풀에서 사용하고 있다(이른바 Half-Sync/Half-Aync타입). Strand를 내부에 사용하고 있어 스레드 풀 모델에서 사용 가능하다.  
다만, 쓰기 처리에 버그가 있고 제대로 사용법을 모르면 사용하기 어려운 것 같다.  


### WebSocket++
[zaphoyd/websocketpp, GitHub](https://github.com/zaphoyd/websocketpp)  
단일 io_service를 사용한다. 설정에 따라서는 Strand를 내부에서 사용하므로 스레드 풀 모델에서 사용 가능하다.


### AZMQ Boost Asio+ZeroMQ
[zeromq/azmq·GitHub](https://github.com/zeromq/azmq) 
단순히 I/O Object을 제공하고 있을 뿐이므로 임의의 모델에서도 사용 가능할 것이다. 구현을 본 느낌으로 단일 스레드로 사용하는 것이 좋을 것 같다.


### Cinder-Asio
[BanTheRewind/Cinder-Asio·GitHub](https://github.com/BanTheRewind/Cinder-Asio)  
단일 io_service를 사용한다 Strand를 내부에 사용하고 있어서 스레드 풀 모델에서 사용 가능하다. 아마 socket의 쓰기 처리에 버그가 있는 듯.


### Simple-Web-Server
[eidheim/Simple-Web-Server·GitHub](https://github.com/eidheim/Simple-Web-Server)  
스레드 풀 모델를 사용한다


### Boost.HTTP
[BoostGSoC14/boost.http·GitHub](https://github.com/BoostGSoC14/boost.http)  
단순히 I/O Object를 제공하고 있는 라이브러리이다. 그런데 Strand와 조합할 수 없기 때문에 단일 스레드에서만 사용 할 수 있다.


### nghttp2-libnghttp2_asio
[tatsuhiro-t/nghttp2·GitHub](https://github.com/tatsuhiro-t/nghttp2)  
io_service-per-쓰레드 모델를 사용한다


### 성능 평가
이렇게 보면 스레드 풀만 사용하는 라이브러리/프레임워크가 압도적으로 많다.  
많은 라이브러리/프레임워크가 스레드 풀 모델을 선택하고 있다면, 스레드 풀을 선택하면 된다고 생각할지도 모른다.
그러나 그 전에 두 모델의 성능을 계측해 보자.  
이 글에서는 몇개 케이스에서 두 모델의 성능을 계측했다.  

아래의 계측에는 물리 기계 Macbook(Early 2015), VirtualBox 위의 Ubuntu14.04 LTS(메모리 1G, CPU x 4)을 사용했다. 컴파일러는 clang 3.6, -std=c++11-stdlib=libc++-pedantic-O3-DNDEBUG 을 옵션으로 지정했다.


#### 쓰레드 수와 1 핸들러 근처의 실행 시간 관계
이 테스트 케이스에서는 미리 io_service에 1,000,000개의 핸들러를 등록하고 io_service::run이 종료할 때까지의 시간을 계측한다. io_service-per-쓰레드 모델의 경우는 합계 1,000,000개의 핸들러를 각 io_service에 골고루 배정하고 있다.
계측에 사용한 코드는 [여기](https://gist.github.com/c11c59e9493a99cda651)이다.  
핸들러의 실행 시간을 조정하기 위한 장치 내에서 빈 루프를 돌리고 있다.  
빈 루프 수가 1,000의 결과가 아래가 된다.
<img src="resource\asio_thread_model_01.png">

io_service-per-쓰레드 모델에서는 스레드 수에 대해서 스케일 하고 있지만(쓰레드 수 3 이상이 미묘하지만), 스레드 풀 모델에서는 스레드 수를 3으로 하면 늦어지고 있다.
이번에는 빈 루프 수가 5,000의 경우의 결과를 보자.
<img src="resource\asio_thread_model_02.png">

어떤 모델도 거의 같은 결과가 되었다.
스레드 풀 모델의 이 결과는 io_service 위에서 스레드 간 동기에 따른 것으로 생각된다.  
핸들러가 가벼우면 시간당 io_service 접속 시간의 비율이 커지기 때문에 스레드 간 경합이 빈발해진다. 핸들러를 늦어지면 io_service에 접속 시간 보다 핸들러 처리 시간이 전체의 시간을 차지하기 때문에 경쟁이 줄어서 동기 비용이 작아졌다고 생각한다.


### 이 밖의 테스트 케이스
이 밖에도 producer/consumer 나 핸들러에서 핸들러를 등록하는 경우도 시험해 봤는데 모두 비슷한 결과가 되었다.  
결과적으로 io_service 동기 비용은 생각보다 큰것을 알았다.



## 정리
- 단일 스레드로 충분하면 단일 스레드 모델로 한다.
    - 단일 스레드로 충분한 퍼포먼스를 얻는다면 괜히 복잡하게 할 필요는 없다.
    - 시간이 드는 처리는 워커 스레드에서 실행하자.
- 스레드 풀 모델보다 io_service-per-쓰레드 모델을 우선한다.
    - 최대한 스루풋를 낸다면 이 모델이다.
    - 많은 라이브러리가 스레드 풀을 선택하지만 신경 쓸 필요는 없다(nghttp2는 잘 알고 있었던 것 같음).
- 스레드 풀 모델은 핸들러의 실행 시간이 짧지 않은 경우에 검토한다.
    - io_service 동기 비용은 작지 않다!!!.
    - cpp-netlib의 비동기 서버는 스레드 풀도 io_service 사이의 메세지 패싱도 하고 있는데, 이것은 시간이 드는 처리를 하는 것을 전제로 한 설계이다.  


io_service 동기 비용은 퍼포먼스에 매우 영향이 크다.  
실제, 실행 시간이 짧은 핸들러가 주요 서버에 cpp-netlib의 흉내를 내고 Half-Sync/Half-Aync 패턴을 구현하면 스레드 수 1이 최고 스루풋라고 하는 안타까운 결과가 되기도 했다. 그러나 그 후 io_service-per-쓰레드 모델로 변경하면 스루풋이 2배 이상 향상 되었다.  

io_service를 사용할 때 대상 특성을 잘 이해하고 용법, 용량을 지켜서 올바르게 사용하자.


### 참고
- [io_service 사용법](http://amedama1x1.hatenablog.com/entry/2015/12/14/000000_1)