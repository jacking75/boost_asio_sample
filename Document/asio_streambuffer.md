## 개념
- std::streambuf를 상속 받는다.
- 복사가 불가능 함(boost::noncopyable 상속)
- 커스텀 할당자를 지원함
- 최대 크기(maximum_size) 인자를 지정해주지 않으면 std::numeric_limits<std::size_t>::max() 값이 지정 됨
- Allocator는 기본 할당자(std::allocator<char>)로 지정
    - Streambuf의 크기는 가변적. 런타임에 크기를 지정하는 것도 가능하다.
    - 최대 크기를 명시적으로 제한할 수 있음
    - 커스텀 할당자를 사용할 수 있다.
- 멤버 변수 부분
```
std::size_t max_size_;
std::vector<char_type, Allocator> buffer_;
```

- 버퍼 내부는 그냥 리니어한 메모리.(std::vector)
- 버퍼는 보통 선두에서 데이터를 나란하도록 관리한다.(즉, consume로 메모리 복사를 하고 나머지 데이터를 버퍼 선두로 가져간다.)



## 멤버 변수
- prepare(size_t n)  
버퍼를 준비하는(prepare) 함수. Receive를 할 때 이 함수를 호출해서 쓰기 버퍼 영역을 만들면 된다. 처음에 봤을 땐 Prepare 자체에서 할당을 하나 했지만 기존에 있는 버퍼를 사용하는거였음. 윈속 프로그래밍을 하다보면 WSABUF라는 구조체를 사용하는데 그것과 같은 기능을 한다고 보면 될 것이다.  
output-sequence의 current-position 포인터를 반환한다.  
반환 값은 mutable-buffer 형식(mutable-buffer의 내부는 pointer와 size의 pair)  
 
- reserve(size_t n)  
쓰기 공간을 예약하는 함수로 내부적으로 버퍼 트리밍하는 기능도 가지고 있다.
[버퍼의시작] … [읽기시작부분]…..[쓰기시작부분]…[버퍼끝] 이렇게 있을 때 쓰기 공간이 부족하면 읽기시작부분~쓰기시작부분을 앞으로 당겨서 공간을 확보하는 기능을 함.
 
- consume(size_t n)  
버퍼에서 데이터를 읽었을 때 읽기 인덱스를 뒤로 당기는 함수. 인자로 전달되는 n만큼 당긴다.  
gptr을 전진한다.
 
- commit(size_t n)  
버퍼에 데이터를 썼을 때 쓰기 인덱스를 뒤로 당기는 함수. 마찬가지로 n만큼 당김  
output-sequence, input-sequence의 포인터를 갱신한다.  
output-sequence의 current-pointer를 갱신한다.  
input-sequence의 begin, current, end 각각의 포인터를 갱신한다.  
 
- data()  
읽기 부분을 반환한다. 나는 버퍼의 시작점을 주는건줄 알았는데 -_-;;  
const 버퍼를 구축하여 반환한다(const_buffer 내부는 const-pointer와 size의 pair)  
const_buffer를 가리키는 포인터는 input-sequence의 current에 셋한다.  
size는 output_current - input_current 이다.
 
- size()  
버퍼에 담겨진 데이터의 양을 반환함.
  
- 공간을 예약(reserve)하는 함수  
    - 버퍼에 데이터를 삽입하기 전, 공간은 충분히 확보가 되어있어야 한다.
    - 따라서 데이터를 삽입을 준비할 때 이 함수가 호출된다.
    - std::length_error 예외를 던진다.
    - 버퍼의 공간을 준비하는 prepare 함수도 내부적으로 reserve를 호출 후 공간을 반환!
    - 트림(trim)의 기능도 있다!
```
void reserve(std::size_t n)
{
    // Get current stream positions as offsets. 얻는다 현 스트림 위치의 오프셋을
    std::size_t gnext = gptr() - &buffer_[0];
    std::size_t pnext = pptr() - &buffer_[0];
    std::size_t pend = epptr() - &buffer_[0];
 
    // Check if there is already enough space in the put area. 검사함 이미 충분히 공간이 확보되었는지
    if (n <= pend - pnext)
    {
      return;
    }
 
    // Shift existing contents of get area to start of buffer. 기존에 있던 데이터들을 버퍼의 시작 지점(&buffer[0)]으로 이동시킴
    if (gnext > 0)
    {
      pnext -= gnext;
      std::memmove(&buffer_[0], &buffer_[0] + gnext, pnext);
    }
 
    // Ensure buffer is large enough to hold at least the specified size. 확인한다 버퍼가  적어도 명시된 크기(n)를 수용할 수 있는지
    if (n > pend - pnext)
    {
      if (n <= max_size_ && pnext <= max_size_ - n)
      {
        pend = pnext + n;
        buffer_.resize((std::max<std::size_t>)(pend, 1));
      }
      else
      {
        std::length_error ex("boost::asio::streambuf too long");
        boost::asio::detail::throw_exception(ex);
      }
    }
 
    // Update stream positions. // 스트림 위치를 갱신함
    setg(&buffer_[0], &buffer_[0], &buffer_[0] + pnext);
    setp(&buffer_[0] + pnext, &buffer_[0] + pend);
}
 
// 데이터 receive 전, 공간을 준비하는 함수이다.
// 인자로 전해지는 n의 크기만큼 공간을 준비한다.
// 내부적으로 reserve 함수를 호출.
mutable_buffers_type prepare(std::size_t n)
{
    reserve(n);
    return boost::asio::buffer(boost::asio::mutable_buffer(
          pptr(), n * sizeof(char_type)));
}
```

- 입출력 시퀸스 관련 변수  
<pre>
                        beginning           current position            end
                    (beginning pointers)    (get/put pointer)       (end pointers)
Input sequence          eback               gptr                    egptr
Output sequence         pbase               pptr                    epptr
</pre>
- pbase  
  write buffer의 처음
- epptr  
  write buffer의 끝
- pptr
  write buffer의 현재 위치
- eback  
  read buffer의 처음
- egptr  
  read buffer의 끝
- gptr  
  read buffer의 현재 위치

- 쓰기 버퍼의 총 량을 반환하는 함수는 다음과 같이 쓸 수 있다
```
size_t capacity() const  
{    
  return epptr() - pbase(); // 쓰기 버퍼의 끝 - 쓰기 버퍼의 처음  
}
```


## boost::asio::streambuf에 custom memory allocator 붙이기
boost::archive, boost::pool_alloc, basic_streambuf의 조합한 사용 예제  

```
// http://zepeh.tistory.com/269

using pool_allocator = boost::pool_allocator<char>;
 
boost::asio::basic_streambuf<pool_allocator> bs;
```


## 사용하기 1
boost.asio에서 데이터를 취득할 때 strambuffer를 사용하여 취득하기도 한다.  
취득한 데이터를 string에 저장하고 싶을 때가 있다(예를 들어 json를 디코딩 하는 등) 그럴 때 도움이 되는 Tip 이다. 
  
일단 http GET 하여 contents를 얻는다.  
asio 샘플 코드를 바탕으로. [asio sample sync]()
  
### 동기
우선 스트림을 그대로 표준 출력에 표시.  

```
// Write whatever content we already have to output.
if (response.size() > 0)
  std::cout << &response;

// Read until EOF, writing data to output as we go.
boost::system::error_code error;
while (boost::asio::read(socket, response,
    boost::asio::transfer_at_least(1), error))
  std::cout << &response;
```

response는 boost::asio::streambuf 타입이다. stream이라 cout로 흘릴 수 있다.  


### istream을 사용

```
boost::asio::streambuf b;

// reserve 512 bytes in output sequence
boost::asio::streambuf::mutable_buffers_type bufs = b.prepare(512);

size_t n = sock.receive(bufs);

// received data is "committed" from output sequence to input sequence
b.commit(n);

std::istream is(&b);
std::string s;
is >> s;
```

똑같이 streambuf을 istream 등을 사용하여 string으로 쓰는 것은 가능하지만 공백 문자가 사라지는 등 데이터에 따라서는 stream 유래한 문제가 발생하므로 이 방법은 NG입니다.  


### buffer_cast
상기의 방법 streambuf.data()에서 mutable_buffer or const_buffer를 취득하여 buffer_cast를 사용한다.   
이 방법으로 stream을 사용하지 않고 string에 데이터를 흘러 넣을 수 있다.  
단 내가 해본 결과 쓰레기 데이터까지 얻어지므로 substr로 정상 데이터까지 잘랐다

```
boost::system::error_code error;
std::string result = "";
while (boost::asio::read(socket, response,
        boost::asio::transfer_at_least(1), error)) {

  std::string tmp = boost::asio::buffer_cast<const char *>(response.data());
  result += tmp.substr(0, response.size());
}
```
  
response.size()로 실제로 받은 데이터 사이즈를 알 수 있으므로 그 길이로 자르고 복사한다. result에 수신 데이터가 다 들어갔다


### 비동기
기본적으로 connect, read, write 함수에 async_를 붙이고 핸들러를 등록하여 콜백 구동으로 하면 간단하게 동작하지만 read()은 동기의 경우 반환 값이 true는 아직 데이터가 존재하고 있었지만 비동기이기 때문에 반환 값은 사용할 수 없다. 대신 boost::system::error_code를 사용한다.  

통상은 succeeded=0 이지만 eof 나 에러시는!=0 이 되므로 if(!err)로 eof의 경우는 처리를 종료한다. 
  
출력할 문자열을 캡쳐 하여 buffer_cast를 사용하면 버퍼를 모두 취득할 수 있다. 

```
void handle_read_content(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Write all of the data that has been read so far.
      std::cout << &response_;

      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      std::cout << "Error: " << err << "\n";
    }
  }
```


### coroutine
비동기로 쓰면 상당히 귀찮다. 그래서 corouine을 사용하면 매우 쉽게 된다.  
  
```
boost::system::error_code ec;
std::string result;

while (!ec){
  boost::asio::async_read(socket, response,
    boost::asio::transfer_at_least(1), yield[ec]);

  std::string tmp = boost::asio::buffer_cast<const char *>(response.data());
  result += tmp.substr(0, response.size());
}
```




### 참고
- [boost::asio::streambuf 분석..](http://zepeh.tistory.com/253)
- [boost::asio::streambuf에 custom memory allocator 붙이기](http://zepeh.tistory.com/269)
- [streambufferからstd::stringへコピー](http://qiita.com/YukiMiyatake/items/8f2ef7bd4e003629828c)