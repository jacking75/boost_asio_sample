#include <SDKDDKVer.h>

#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>


void OnTimer( const boost::system::error_code& error, boost::asio::steady_timer* pTimer );

int nCount = 0;

void SetTimer( boost::asio::steady_timer* pTimer )
{
	std::cout << "SetTimer: " << time(NULL) << std::endl;

	pTimer->expires_from_now( boost::chrono::milliseconds(1000)); 

    pTimer->async_wait( boost::bind( OnTimer, 
									boost::asio::placeholders::error, 
									pTimer
								)							
					);
}

void OnTimer( const boost::system::error_code& error, boost::asio::steady_timer* pTimer )
{
    if( !error ) 
	{
		++nCount;

        std::cout << "OnTimer: " << nCount << std::endl;

		if( nCount < 5 )
		{
			SetTimer( pTimer );
		}
    }
	else
	{
		std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
	}
}




int main()
{
	boost::asio::io_service io_service;

    boost::asio::steady_timer timer(io_service); 

	SetTimer( &timer );
	   
    io_service.run();

	std::cout << "Á¾·á: " << time(NULL) << std::endl;
	return 0;
}
