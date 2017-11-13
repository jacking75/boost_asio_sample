### 코루틴 사용
Asio는 네트워크 통신으로 주로 사용하지만 비동기 I/O 라이브러리이므로 네트워크 이외에도 사용한다. 
  
예를 들면 1초마다 Update 함수를 부르는 무한 루프는
```
boost::asio::spawn(io_service_, [&self](boost::asio::yield_context yield) {
  for (; ;) {
    timer.expires_from_now(boost::posix_time::seconds(1));
    timer.async_wait(yield[ec]);

    Update();
  }
});
```
이렇게 간단하게 구현할 수 있다.
  
이와 비슷한 방식을 코루틴 없이 구현하면
```
// 출처: http://faithandbrave.hateblo.jp/entry/20110530/1306739714  

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

boost::mutex mutex;

template <class T>
void write_console(const T& x)
{
    boost::mutex::scoped_lock lock(mutex);
    std::cout << x << std::endl;
}

class event_manager {
    boost::asio::io_service io_service_;
    boost::shared_ptr<boost::asio::io_service::work> work_;
    boost::thread_group group_;

    boost::asio::strand ok_button_strand_;
    boost::asio::strand cancel_button_strand_;

    boost::function<void()> ok_button_clicked_;
    boost::function<void()> cancel_button_clicked_;
public:
    event_manager()
        : ok_button_strand_(io_service_),
          cancel_button_strand_(io_service_)
    {
        work_.reset(new boost::asio::io_service::work(io_service_));

        for (std::size_t i = 0; i < 3; ++i) { // 3スレッドで動かす
            group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
        }
    }

    ~event_manager()
    {
        work_.reset();
        group_.join_all();
    }

    template <class F>
    void set_ok_button_event(F f) { ok_button_clicked_ = f; }

    template <class F>
    void set_cancel_button_event(F f) { cancel_button_clicked_ = f; }

    void ok_button_click()
    {
        // 排他可能にしてpost
        io_service_.post(ok_button_strand_.wrap(ok_button_clicked_));
    }

    void cancel_button_click()
    {
        io_service_.post(cancel_button_strand_.wrap(cancel_button_clicked_));
    }
};


class Button {
    std::string name;
public:
    explicit Button(const std::string& name)
        : name(name) {}

    void clicked()
    {
        write_console("start : " + name + " clicked");
        boost::this_thread::sleep(boost::posix_time::seconds(2));
        write_console("end : " + name + " clicked");
    }
};

int main()
{
    event_manager manager;

    Button ok_button("ok");
    Button cancel_button("cancel");

    manager.set_ok_button_event(boost::bind(&Button::clicked, &ok_button));
    manager.set_cancel_button_event(boost::bind(&Button::clicked, &cancel_button));

    // ボタンのクリックイベント中に同じイベントを発生させようとする
    manager.ok_button_click();
    manager.ok_button_click();
    manager.cancel_button_click();
    manager.cancel_button_click();
    manager.ok_button_click();

    for (;;) {}
}
```