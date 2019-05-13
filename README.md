# boost_asio_sample
- [ebook]Boost.Asio를 이용한 네트워크 프로그래밍(http://www.hanbit.co.kr/store/books/look.php?p_code=E7889843127) 의 예제 코드.   
- 책에는 없는 예제 코드.  
- (2019.05.01)Samples_VS2019 디렉토리에는 Visual Studio 2019 지원.
    - [boost/asio deprecated 1.69.0](https://zepeh.tistory.com/498 )을 보고 새로운 API 사용.
  
  
## 문서 
[Boost.Asio의 원리 원칙](Document/asio_principle.md)     
[Boost.Asio의 프로그래밍 기본](Document/asio_basic.md)  
[Boost.Asio 커스텀 할당기](Document/asio_allocation.md)  
  
[C++11 프로그래밍](Document/asio_cpp11.md)  
- C++11의 lambda를 사용한 비동기 완료 함수 등록
- future 사용하기
- std::packaged_task 사용하기
    
[Boost.Asio와 코루틴](Document/asio_coroutine.md)  
[Boost.Asio와 이벤트 핸들러](Document/asio_eventHandler.md)    
[Boost.Asio streambuffer](Document/asio_streambuffer.md)  
[Boost.Asio 스레드 모델](Document/asio_thread_model.md)  
[Boost.Asio 워커 스레드 패턴](Document/worker_thread_pattern.md)    
  
[Boost.Asio로 작성한 http client](Document/http_client.md)    
[Boost.Asio linux에서 사용하기](Document/asio_linux.md)  
[Boost.Asio Tips](Document/asio_tips.md)
- 자기 자신의 IP 얻기
- ip::address를 사용하여 IP주소를 문자열, 정수, 바이트 배열로 변환하기  
- windows::object_handle 다루기
- shared_ptr 캡쳐
- 타임 아웃 처리 
  
  
## boost/asio deprecated
C++ 표준을 위해 Asio의 일부 API가 변경됨.  
자세한 설명은 [boost/asio deprecated 1.69.0](https://zepeh.tistory.com/498 ). 
전처리기에 BOOST_ASIO_NO_DEPRECATED를 지정하면 더이상 사용되지 않는 코드들이 비활성화 된다.
- io_service가 io_context로 이름 변경
- io_context::dispatch가 dispatch 로 대체 됨
- io_context::post가 post 로 대체 됨
- io_context::strand::wrap이 bind_execuator 로 대체 됨
- io_context::get_io_context(), io_context::get_io_service()가 context() 함수로 이름 변경.
- io_context::strand::get_io_context(), io_context::strand::get_io_service()가 context() 함수로 이름 변경.
- ip::address::from_string이 ip::make_address로 대체 됨
  
책의 예제 코드에서 바뀐 예  
```
boost::asio::io_context io_service;


boost::asio::io_context io_service;
io_service.post( Function );

m_ioservice.post(boost::bind(&task_t::operator(), task));


boost::asio::io_context::strand st( io_service );
JobManager.post(boost::asio::bind_executor(st, boost::bind(Function, 11) ) );
JobManager.post(boost::asio::bind_executor(st, boost::bind(Function, 12) ) );


boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(SERVER_IP), PORT_NUMBER);
boost::asio::ip::udp::endpoint( boost::asio::ip::make_address(UDP_IP), CLIENT_PORT_NUMBER )
```  
  