#include <SDKDDKVer.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <list>
#include <boost/bind.hpp>
#include <boost/asio.hpp>


const char UDP_IP[] = "127.0.0.1";
const unsigned short SERVER_PORT_NUMBER = 31400;
const unsigned short CLIENT_PORT_NUMBER = 31401;

class UDP_Server
{
public:
	UDP_Server( boost::asio::io_service& io_service )
		: m_Socket(io_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), SERVER_PORT_NUMBER))
	{
		PostReceive();
	}

	~UDP_Server()
	{
	}

	
	void PostReceive()
	{
		memset( &m_ReceiveBuffer, '\0', sizeof(m_ReceiveBuffer) );

		m_Socket.async_receive_from
				( 
					boost::asio::buffer(m_ReceiveBuffer, 128), 
					m_SenderEndpoint, 
					boost::bind( &UDP_Server::handle_receive, 
								this, 
								boost::asio::placeholders::error, 
								boost::asio::placeholders::bytes_transferred ) 
				);
							
	}


private:
	void handle_write(const boost::system::error_code& /*error*/, size_t /*bytes_transferred*/)
	{
	}

	void handle_receive( const boost::system::error_code& error, size_t bytes_transferred )
	{
		if( error )
		{
			std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
		}
		else
		{
			const std::string strRecvMessage = m_ReceiveBuffer.data();
			std::cout << "클라이언트에서 받은 메시지: " << strRecvMessage << ", 받은 크기: " << bytes_transferred << 
				", From: " << m_SenderEndpoint.address().to_string().c_str() << " " << m_SenderEndpoint.port() << 
				std::endl;

			char szMessage[128] = {0,};
			sprintf_s( szMessage, 128-1, "Re:%s", strRecvMessage.c_str() );
			m_WriteMessage = szMessage;
						
			m_Socket.async_send_to( boost::asio::buffer(m_WriteMessage),
				boost::asio::ip::udp::endpoint( boost::asio::ip::address::from_string(UDP_IP), CLIENT_PORT_NUMBER ),
								    boost::bind( &UDP_Server::handle_write, 
												this,
												boost::asio::placeholders::error,
												boost::asio::placeholders::bytes_transferred )
								);

			
			PostReceive(); 
		}
	}

	boost::asio::ip::udp::socket m_Socket;
	boost::asio::ip::udp::endpoint m_SenderEndpoint;

	std::string m_WriteMessage;
	std::array<char, 128> m_ReceiveBuffer;
	
	int m_nSeqNumber;
};

int main()
{
	std::cout <<  "UDP 서버 실행..." << std::endl;

	boost::asio::io_service io_service;
    
	UDP_Server server(io_service);
    
	io_service.run();
  
	std::cout << "네트웍 종료" << std::endl;

	getchar();
	return 0;
}