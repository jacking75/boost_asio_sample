#include <SDKDDKVer.h>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

void Function()
{
	std::cout << "Call Function !!!" << std::endl;
	//::Sleep(1000);
}

void OnTimer1( const boost::system::error_code& error )
{
	std::cout << "Call OnTimer1 !!! " << time(NULL) << std::endl;
}

int main()
{
	std::cout << "시작: " << time(NULL) << std::endl;

    boost::asio::io_service io_service;
	io_service.post( Function );
	io_service.post( Function );
	io_service.post( Function );
	
	boost::asio::steady_timer timer(io_service); 
	timer.expires_from_now( boost::chrono::milliseconds(2000) ); 
	timer.async_wait( OnTimer1 );
	
	io_service.poll();
	
	std::cout << "종료: " << time(NULL) << std::endl;

	getchar();
	return 0;
}

