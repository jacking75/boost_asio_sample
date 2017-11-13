## C++11. Asio와 코루틴
Boost 라이브러리는 1.53 버전부터 코루틴이 정식으로 포함되었다. Boost 라이브러리 1.54 버전의 Asio에서는 코루틴을 Asio에서도 사용할 수 있게 되었다.

### stackless 코루틴
- 간단하게 사용할 수 있다.
- asio::coroutine를 상속한다
- 적용 범위는 #include <boost/asio/yield.hpp> ~ #include <boost/asio/unyield.hpp> 를 포함
- coroutine 함수는 operator()으로
    - operator() 이외도 가능할지도...
- coroutine 부분은 reenter()으로 감싼다
- yield  fork 등을 키워드처러 사용할 수 있다

- asio::coroutine을 상속하기
```
class server : boost::asio::coroutine {
```
- 콜백 함수 오브젝트
```
void operator(...)
```
- fork시에 생성자가 호출되지 않으므로 shared_ptr 등을 사용하면 초기화 하기 쉽다
```
std::shared_ptr<tcp::acceptor> acceptor_;
std::shared_ptr<tcp::socket> socket_;
std::shared_ptr<std::array<char, 1024> > buffer_;
```
- operator(), reenter
    - 코루틴이 재개되는 곳
    - CPU가 할당 될 때 등록한 함수 오브젝트 핸들러가 호출된다
    - reenter 내의 전 회중 중단한 곳에서 재개된다
```
void server::operator()(boost::system::error_code ec, std::size_t length)
{
  if (!ec)
  {
    reenter(this){
```

- fork를 사용하면 새로운 의사 컨텍스트를 만들 수 있다
- is_parent로 부모를 판정할 수 있다
- 새로운 컨텍스트는 개별 멤버를 할당된다(그렇지만 생성자는 fork에서 호출되지 않는다)
```
do {    
  socket_.reset(new tcp::socket(acceptor_->get_io_service()));   
  yield acceptor_->async_accept(*socket_, *this);    
  fork server(*this)();
  } while (is_parent());
```

- yield
    - 비동기 처리를 post 하고 CPU 개시한다
    - 비동기 처리 완료 시에 지정한 함수 오브젝트가 콜백된다.
```
yield socket_>async_read_some(boost::asio::buffer(*buffer_), *this);

yield boost::asio::async_write(*socket_, boost::asio::buffer(*buffer_), *this);
```

- 예
```
// coroutine을 사용하지 않는 예
asio::async_read(socket_, asio::buffer(buffer_),
    [this](boost::system::error_code ec, std::size_t ){
	        asio::async_write(socket_, asio::buffer(buffer_),
		             [this](boost::system::error_code ec, std::size_t ){
                 socket_.shutdown(tcp::socket::shutdown_both, ec);
		             });
	        }       
   });

// stackless_coroutine을 사용한 예
reenter(this){
    do{
        yield acceptor_->async_accept(socket_,*this);
       fork server(*this)();
   }while(is_parent());

    yield socket_.async_read_some(asio::buffer(buffer_), *this);    yield socket.async_write_some(asio::buffer(buffer_), *this);    socket_.shutdown(tcp::socket::shutdown_both, ec);};

```

- 장점
    - 아주 간단하게 사용할 수 있다
    - 꽤 가벼운 듯(switch case로 구현 되어 있다)
    - 콜백 지옥이 없고, 일견 직렬 처리로 되어 소스 가독성도 아주 좋아진다
    - 헤더 추가만으로 라이브러리 추가는 불필요
- 단점
    - switch case로 구현되어 있어서 지역 변수를 다루는데 제한이 있다


### stackful 코루틴
Asio에서 코루틴을 사용하기 위해서는 boost:asio::spawn을 사용한다.

Asio에서 비동기 I/O 함수를 사용할 때는 언제나 Asio의 비동기 함수를 호출하고, 이때 완료 함수를 등록 한다. 그리고 완료 함수가 호출에 의해 작업의 완료를 알게 된다.
<img src="resource\asio_spawn_01.png">

boost::asio::spawn을 사용하면 코루틴에 의해 비동기 함수를 호출하면 호출한 곳으로 제어권을 넘긴 후 비동기 작업이 완료하면 비동기 함수 호출 이후 부분에 복귀하여 남은 작업을 처리한다.
아래는 boost::asio::spawn을 사용하여 비동기로 데이터를 보내는 코드이다.
<img src="resource\asio_spawn_02.png">


< boost::asio::spawn을 사용하여 비동기로 데이터 보내기 >  
코루틴을 사용해본 경험이 없다면 아마 위의 코드만으로는 잘 이해가 안될 것이다. 그러니 기존의 Chatting 서버를 boost::asio::spawn 사용 버전으로 만든 'ChattingTCPServer_spawn' 프로젝트를 꼭 보고 실행까지 해보기 바란다. boost::asio::spawn을 사용해서 코드가 이전에 비해서 줄어들어서 어렵지 않게 이해할 수 있을 것이다.  
[ChattingTCPServer_spawn.zip](resource\ChattingTCPServer_spawn.zip)

- Boost 라이브러리의 asio::spawn 관련 예제 코드
http://www.boost.org/doc/libs/1_54_0/doc/html/boost_asio/example/cpp11/spawn/


#### 타임아웃 처리
coroutine 시의 timeout 처리
```
asio::deadline_timer_;
timer_.expires_from_now(boost::posix_time::seconds(10));
timer_.async_wait(        
  [&socket_](const boost::system::error_code &ec) {            
    if (ec == boost::asio::error::operation_aborted) {                
      cout << “cancel” << endl ;            
      } else {                
        socket_.cancel();                
        cout << “timeout” << endl ;            
      }        
    });

socket_.async_read_some(boost::asio::buffer(buffer_), yield[ec]);
timer_.cancel();
```
