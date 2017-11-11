#include <SDKDDKVer.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>


void OnTimer1( const boost::system::error_code& error )
{
    if( !error ) 
	{
        std::cout << "Call OnTimer1 !!! " << time(NULL) << std::endl;
    }
	else
	{
		std::cout << "OnTimer1. error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}

void OnTimer2( const boost::system::error_code& error )
{
    if( !error ) 
	{
        std::cout << "Call OnTimer2 !!! " << time(NULL) << std::endl;
    }
	else
	{
		std::cout << "OnTimer2. error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}

int main()
{
	std::cout << "시작: " << time(NULL) << std::endl;

    boost::asio::io_service io_service;

    boost::asio::steady_timer timer(io_service); 

    timer.expires_from_now( boost::chrono::milliseconds(2000) ); 

    timer.async_wait( OnTimer1 );
	timer.async_wait( OnTimer2 );

	const int count = timer.cancel();
	std::cout << "타이머 취소 개수: " << count << std::endl;

    io_service.run();

	std::cout << "종료: " << time(NULL) << std::endl;
	return 0;
}