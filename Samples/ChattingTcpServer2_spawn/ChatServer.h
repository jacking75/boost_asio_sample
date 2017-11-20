// Âü°í: https://github.com/chriskohlhoff/asio/blob/master/asio/src/examples/cpp11/chat/chat_server.cpp

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>

#include <utility>



#include "Message.h"
#include "Session.h"
#include "Room.h"



const unsigned short SERVER_PORT = 32452;



class chat_server
{
public:
	chat_server(boost::asio::io_service& io_context,
		const tcp::endpoint& endpoint)
		: acceptor_(io_context, endpoint)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		tcp::socket socket(acceptor_.get_io_service());

		acceptor_.async_accept(socket, 
			[&socket, this](std::error_code ec)
		{
			if (!ec)
			{
				std::make_shared<chat_session>(std::move(socket), room_)->start();
			}

			do_accept();
			
		});
	}


	tcp::acceptor acceptor_;
	
	chat_room room_;
};