핸들러의 메모리 확보&해제 비용은 작지만 가급적이면 커스텀 할당기를 만들어 비용을 낮추면 좋다.  
Boost.Asio 문서에 예제로 있다. [예제](http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio/example/cpp11/allocation/server.cpp)  



```
inline custom_alloc_handler<Handler> make_custom_alloc_handler(
    handler_allocator& a, Handler h)
{
  return custom_alloc_handler<Handler>(a, h);
}

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_),
        make_custom_alloc_handler(allocator_,
          [this, self](boost::system::error_code ec, std::size_t length)
          {
            if (!ec)
            {
              do_write(length);
            }
          }));
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        make_custom_alloc_handler(allocator_,
          [this, self](boost::system::error_code ec, std::size_t /*length*/)
          {
            if (!ec)
            {
              do_read();
            }
          }));
  }

  // The socket used to communicate with the client.
  tcp::socket socket_;

  // Buffer used to store data received from the client.
  std::array<char, 1024> data_;

  // The allocator to use for handler-based custom memory allocation.
  handler_allocator allocator_;
};
```

```
// custom_alloc_handler

class custom_alloc_handler
{
public:
  custom_alloc_handler(handler_allocator& a, Handler h)
    : allocator_(a),
      handler_(h)
  {
  }

  template <typename ...Args>
  void operator()(Args&&... args)
  {
    handler_(std::forward<Args>(args)...);
  }

  friend void* asio_handler_allocate(std::size_t size,
      custom_alloc_handler<Handler>* this_handler)
  {
    return this_handler->allocator_.allocate(size);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t /*size*/,
      custom_alloc_handler<Handler>* this_handler)
  {
    this_handler->allocator_.deallocate(pointer);
  }

private:
  handler_allocator& allocator_;
  Handler handler_;
};
```

```
// handle_allocator

class handler_allocator
{
public:
  handler_allocator()
    : in_use_(false)
  {
  }

  handler_allocator(const handler_allocator&) = delete;
  handler_allocator& operator=(const handler_allocator&) = delete;

  void* allocate(std::size_t size)
  {
    if (!in_use_ && size < sizeof(storage_))
    {
      in_use_ = true;
      return &storage_;
    }
    else
    {
      return ::operator new(size);
    }
  }

  void deallocate(void* pointer)
  {
    if (pointer == &storage_)
    {
      in_use_ = false;
    }
    else
    {
      ::operator delete(pointer);
    }
  }

private:
  // Storage space used for handler-based custom memory allocation.
  typename std::aligned_storage<1024>::type storage_;

  // Whether the handler-based custom allocation storage has been used.
  bool in_use_;
};
```

핸들러의 메모리 확보&해제 때 allocate deallocate가 호출된다.  
typename std::aligned_storage<1024>(type storage_;  
로 1024 Bytes의 메모리를 확보하고 있다.  
메모리 얼라이먼트 때문에 aligned_storage로 확보.  
이 샘플에서는 이 handler_allocator는 session에서 만들어서 참고하고 있으므로
session 내에서 몇번이라도 async 함수를 사용해도 다시 할당하지 않는다.  
그리고 핸들러로 확보해야 하는 영역이 1024 Bytes 이하의 경우는 storage_ 에 덮어쓰기를 하느라 new delete가 이루어지지 않아서 퍼포먼스 향상.  
1024 Bytes를 넘으면 new delete를 사용.


### 출처
- [async関数のハンドラ](http://qiita.com/YukiMiyatake/items/36bd59171f39b6bef6b3)