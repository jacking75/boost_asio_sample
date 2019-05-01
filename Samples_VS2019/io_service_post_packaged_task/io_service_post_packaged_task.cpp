// http://yamasv.blog92.fc2.com/blog-entry-183.html

#include <boost/asio.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/thread.hpp>


class Servant
{ 
	boost::asio::io_context m_ioservice; 
    boost::asio::io_context::work m_work; 
    boost::thread_group m_threads; 

public:
	Servant() : m_ioservice(), m_work(m_ioservice), m_threads() 
	{ 
        m_threads.create_thread( boost::bind(&boost::asio::io_context::run, &m_ioservice) ); 
        std::cout << "백그라운드 스케쥴러 시작" << std::endl;
    }; 

	~Servant() 
	{ 
        m_ioservice.stop(); 
        m_threads.join_all(); 
    }; 


	boost::unique_future<int> do_something(const std::string& str) 
	{
		return schedule(boost::bind(&Servant::do_something_impl, this, str));
    }; 

private:
	template <typename T> 
    boost::unique_future<int> schedule(T function) 
	{ 
		typedef boost::packaged_task<int> task_t;
        std::shared_ptr<task_t> task = std::make_shared<task_t>(function);

        boost::unique_future<int> f = task->get_future();

        m_ioservice.post(boost::bind(&task_t::operator(), task));
        std::cout << "백그라운드 스케쥴러에 작업을 요청했다" << std::endl;

        return f;
    }; 


    int do_something_impl(const std::string& str) 
	{ 
		std::cout << "시간이 꽤 걸리는 작업 시작...." << std::endl;

        boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
        std::cout << str << std::endl;
        return 777;
    }; 
}; 


int main()
{
	Servant s;
	boost::unique_future<int> f = s.do_something("일감 1"); 

    std::cout << "작업이 끝날 때까지 대기한다." << std::endl;
    
	std::cout << f.get() << std::endl;

    return 0;
}  

