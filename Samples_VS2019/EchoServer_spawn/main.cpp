// Âü°í: http://www.boost.org/doc/libs/1_65_1/doc/html/boost_asio/example/cpp11/spawn/echo_server.cpp

#include "EchoServer.h"

int main()
{
	try
	{		
		boost::asio::io_context io_service;

		boost::asio::spawn(io_service,
			[&](boost::asio::yield_context yield)
				{
					tcp::acceptor acceptor(io_service,
										tcp::endpoint(tcp::v4(), SERVER_PORT)
					);

					for (;;)
					{		
						std::cout << "accept loop...." << std::endl;

						tcp::socket socket(io_service);

						boost::system::error_code ec;
						acceptor.async_accept(socket, yield[ec]);

						if (!ec) 
						{
							std::cout << "new session" << std::endl;

							std::make_shared<session>(std::move(socket), io_service)->go();
						}
					}
				}
		);

		std::cout << "call io_service.run()" << std::endl;

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}