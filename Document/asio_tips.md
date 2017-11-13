### 자기 자신의 IP 얻기
자기 자신(컴퓨터)의 IP 어드레스는 다음과 같은 방법으로 얻을 수 있다.
처음에는 IPv6 주소, 두 번째는 IPv4 주소가 나온다.

```
std::cout << "1" << std::endl;
boost::asio::io_service io_service;
boost::asio::ip::tcp::resolver resolver(io_service);
boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(), "");
boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
boost::asio::ip::tcp::resolver::iterator end; // End marker.
while (iter != end)
{
	boost::asio::ip::tcp::endpoint ep = *iter++;
	std::cout << ep << std::endl;
}

{
	std::cout << "2" << std::endl;
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(),"");
	boost::asio::ip::tcp::resolver::iterator it=resolver.resolve(query);

	while(it!=boost::asio::ip::tcp::resolver::iterator())
	{
		boost::asio::ip::address addr=(it++)->endpoint().address();
		if(addr.is_v6())
		{
			std::cout<<"ipv6 address: ";
		}
		else
			std::cout<<"ipv4 address: ";

		std::cout<<addr.to_string()<<std::endl;

	}
}
```



### ip::address를 사용하여 IP주소를 문자열, 정수, 바이트배열로 변환하기

```
auto ipp = boost::asio::ip::address_v4::from_string("192.168.1.2");
std::cout << "to_string()  " << ipp.to_string() << std::endl;
std::cout << "to_ulong()  " << ipp.to_ulong() << std::endl;
auto bt(ipp.to_bytes());
std::cout << "to_bytes()  " << (int)bt[0] << "." << (int)bt[1] << "." << (int)bt[2] << "." << (int)bt[3]  << std::endl;
```


### windows::object_handle 다루기
Boost.Asio의 장점 중의 하나가 멀티 플랫폼을 지원하면서 각 플랫폼의 특징을 죽이지 않고 사용할 수 있도록 해주는 것이다.  
Boost 라이브러리 1.49.0 버전에서 Windows에서 HANDLE의 비동기 조작을 할 수 있도록 해주는 windows::object_handle 클래스가 추가 되었다.  
아래의 코드는 프로세스를 비동기로 실행 후 종료할 때까지 대기 하다가 프로세스가 종료하면 등록된 함수를 호출한다.  

```
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/windows/object_handle.hpp>

namespace asio = boost::asio;

void on_end_process(const boost::system::error_code& ec)
{
    if (!ec)
       {
        std::cout << "end process" << std::endl;
    }
}

int main()
{
    asio::io_service io_service;

    PROCESS_INFORMATION pi = {};
    STARTUPINFO si = {};

    si.cb = sizeof(si);

    std::string proc = "notepad";
       CreateProcessA(NULL, &proc[0], NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, (LPSTARTUPINFOA)&si, &pi);

    asio::windows::object_handle process(io_service, pi.hProcess);
    process.async_wait(on_end_process);

    io_service.run();
}
```

object_handle에는 wait/async_wait()가 있으며 내부에서 WaitForSingleObjecr를 호출한다. 프로세스 이외에도 CreateEvent로 만든 핸들을 SetEvent 될 때까지 대기시킬 때도 사용할 수 있다.

출처: http://d.hatena.ne.jp/faith_and_brave/20120229/1330496131


### shared_ptr 캡쳐

```
class session {    
	void do_read() {        
		async_read( socket_,buffer,[this](){this->hoge;});    
	}
}
```
비동기이므로 핸들러 실행 시에 this는 메모리 해제되을수도 있어서 핸들러를 실행하려는 순간에 오브젝트 끝날 수 있다.  

```
class session::enable_shared_from_this<session> {    
	void do_read() {
		    auto self(shared_from_this());
				async_read( socket_,buffer,[this, self](){this->hoge;});    
			}
}
```
자신의 shared_ptr을 캡쳐하여 핸들러 종료까지 오브젝트를 끝내지 않는다



### 타임 아웃 처리 
출처: http://qiita.com/YukiMiyatake/items/0ce70544202315450533

```
// ライブラリ
template<class T>
void deadlineOperation3(T &t,
  const unsigned int timeout_ms) {

  t.deadline_timer_.expires_from_now( boost::posix_time::milliseconds(timeout_ms));
  t.deadline_timer_.async_wait(
    [=](const boost::system::error_code &ec) {
      if (ec != boost::asio::error::operation_aborted) {
        t.handle_timeout(ec);    // タイムアウトしたときのハンドラ
      }
    });
}


deadlineOperation3<ThisClass>(this, timeout_ms_);

boost::asio::async_read(socket_, response_,
    boost::asio::transfer_at_least(1),
    [this, self](const boost::system::error_code& ec, std::size_t bytes_transferred) {
      deadline_timer_.cancel();
      handle_read_(ec);
    });
);
```

타임 아웃 클래스를 shared_ptr 로 하여 destructor 에서 타이머 취소한다
```
// ライブラリ
class deadlineOperation3 /*: public std::enable_shared_from_this<deadlineOperation3>*/{
private:
  boost::asio::deadline_timer deadline_timer_;
public:
  deadlineOperation3(boost::asio::io_service &io_service, unsigned int timeout_ms
    , std::function<void(const boost::system::error_code &)> handle_timeout)
    : deadline_timer_(io_service) {

  deadline_timer_.expires_from_now( boost::posix_time::milliseconds(timeout_ms));

  deadline_timer_.async_wait(
    [=](const boost::system::error_code &ec) {
      if (ec != boost::asio::error::operation_aborted) {
    handle_timeout(ec);
      }
    });
  };

  virtual ~deadlineOperation3() {
    // デストラクタでタイマーキャンセルする
    deadline_timer_.cancel();
  };
};


// 使う側
// shared_ptrを使い タイムアウトクラス生成。タイムアウト時のコールバック登録
auto timer = std::make_shared<asioUtil::deadlineOperation3>(io_service_, 1000,
[this,self](const boost::system::error_code &ec) {
  socket_.cancel();
  handle_timeout();
});

boost::asio::async_read(socket_, response_,
boost::asio::transfer_at_least(1),
// timerをキャプチャする。コールバックを抜けるとdestructerにより タイマーがキャンセルされる仕組み
[this, self, timer](const boost::system::error_code& ec, std::size_t bytes_transferred) {
  handle_read(ec);
});

```