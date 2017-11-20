// Âü°í: https://github.com/chriskohlhoff/asio/blob/master/asio/src/examples/cpp11/chat/chat_server.cpp

#include "ChatServer.h"


int main()
{
	try
	{
		
		boost::asio::io_service io_context;

		tcp::endpoint endpoint(tcp::v4(), SERVER_PORT);
		
		chat_server servers(io_context, endpoint);
		

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}