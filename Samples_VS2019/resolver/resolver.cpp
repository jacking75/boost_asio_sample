#include <SDKDDKVer.h>

#include <iostream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>


void OnResolve( const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator endpoint_iterator )
{
	if (error) 
	{
		std::cout << error.message() << std::endl;
	}
	else 
	{
		boost::asio::ip::tcp::resolver::iterator end;
	
		for (; endpoint_iterator != end; ++endpoint_iterator) 
		{
			std::cout << "IP: " << endpoint_iterator->endpoint().address().to_string() << std::endl;
		}
	}
}


int main()
{
	boost::asio::io_context io_service;

	boost::asio::ip::tcp::resolver resolver(io_service);

	boost::asio::ip::tcp::resolver::query query("google.com", "http");
	resolver.async_resolve( query, boost::bind( OnResolve,
											boost::asio::placeholders::error, 
											boost::asio::placeholders::iterator )
							);

	io_service.run();

	getchar();
	return 0;
}
