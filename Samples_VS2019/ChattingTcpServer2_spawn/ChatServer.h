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



const unsigned short SERVER_PORT = 31452;



class chat_server
{
public:
	chat_server(boost::asio::io_context& io_context,
		const tcp::endpoint& endpoint)
		: acceptor_(io_context, endpoint)
		, m_io_context(io_context)
		, m_NewSocket(io_context)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept(m_NewSocket,
			[this](std::error_code ec)
			{
				if (!ec)
				{
					std::cout << "New Connected" << std::endl;
					std::make_shared<chat_session>(std::move(m_NewSocket), room_)->start();
				}
				else
				{
					std::cout << ec.message() << std::endl;
				}

				do_accept();

			});
	}


	tcp::acceptor acceptor_;
	boost::asio::io_context& m_io_context;
	tcp::socket m_NewSocket;
	
	chat_room room_;
};