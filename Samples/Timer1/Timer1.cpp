#include <SDKDDKVer.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>


void OnTimer( const boost::system::error_code& error )
{
    if( !error ) 
	{
        std::cout << "Call OnTimer!!! " << time(NULL) << std::endl;
    }
	else
	{
		std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}

int main()
{
	std::cout << "시작: " << time(NULL) << std::endl;

    boost::asio::io_service io_service;

    boost::asio::steady_timer timer(io_service); 

    timer.expires_from_now( boost::chrono::milliseconds(2000) ); 

    timer.async_wait( OnTimer );

    io_service.run();

	std::cout << "종료: " << time(NULL) << std::endl;
	return 0;
}