## Boost.Asio 에 의한 워커 스레드 패턴

- [출처](http://faithandbrave.hateblo.jp/entry/20110408/1302248501)

```
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

class thread_pool {
    boost::asio::io_service& io_service_;
    boost::shared_ptr<boost::asio::io_service::work> work_;
    boost::thread_group group_;
public:
    thread_pool(boost::asio::io_service& io_service, std::size_t size)
        : io_service_(io_service)
    {
        work_.reset(new boost::asio::io_service::work(io_service_));

        for (std::size_t i = 0; i < size; ++i) {
            group_.create_thread(boost::bind(&boost::asio::io_service::run, &io_service_));
        }
    }

    ~thread_pool()
    {
        work_.reset();
        group_.join_all();
    }

    template <class F>
    void post(F f)
    {
        io_service_.post(f);
    }
};

// test
#include <iostream>
#include <string>
#include <boost/format.hpp>

boost::mutex g_mutex;

void func(const std::string& str, int num, int count)
{
    int n = 0;
    for (int i = 0; i < num; i++)
    {
        n += count;
        {
            boost::mutex::scoped_lock lock(g_mutex);
            std::cout << (boost::format("%1% %2%/%3% (%4%)") % str % i % num % ::GetCurrentThreadId()) << std::endl;
        }
        ::Sleep(1000);
    }
    {
        boost::mutex::scoped_lock lock(g_mutex);
        std::cout << (boost::format("%1% %2%/%2% (%3%) result: %4%") % str % num % ::GetCurrentThreadId() % n) << std::endl;
    }
}


class hoge
{
private:
    int num_;
    int count_;

public:
    hoge(int num, int count) : num_(num), count_(count) { }
    void func(const std::string& str) { ::func(str, num_, count_); }
};

int main()
{
    boost::asio::io_service io_service;
    thread_pool tp(io_service, 3);

    tp.post(boost::bind(func, "request1:", 5, 10));
    tp.post(boost::bind(func, "request2:", 5, 10));
    ::Sleep(3000);
    hoge h(3, 5);
    tp.post(boost::bind(&hoge::func, &h, "request3:"));
    tp.post(boost::bind(&hoge::func, &h, "request4:"));
    ::Sleep(7000);
}
```