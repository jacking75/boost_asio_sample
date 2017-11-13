#include "ServerSession.h"
#include "ChattingServer.h"

#include <boost/asio/spawn.hpp>


Session::Session(int nSessionID, boost::asio::io_service& io_service, ChatServer* pServer)
		: m_Socket(io_service)
		, m_nSessionID( nSessionID )
		, m_pServer( pServer )
{
	m_bCompletedWrite = true;
}

Session::~Session()
{
	while( m_SendDataQueue.empty() == false )
	{
		delete[] m_SendDataQueue.front();
		m_SendDataQueue.pop_front();
	}
}

void Session::Init()
{
	m_nPacketBufferMark = 0;
	m_IsPostReceive = false;
}

void Session::Send(const int nSize, char* pData)
{
	auto pSendData = new char[nSize];
	memcpy( pSendData, pData, nSize);

	boost::asio::spawn(m_Socket.get_io_service(),
        [this,pSendData](boost::asio::yield_context yield)
        {
			PACKET_HEADER* pHeader = (PACKET_HEADER*)pSendData;

			boost::system::error_code ec;
			boost::asio::async_write(m_Socket, boost::asio::buffer(pSendData, pHeader->nSize), yield[ec]);

			if(ec.value() > 0)
			{
				std::cout << "error No: " << ec.value() << " error Message: " << ec.message() << std::endl;
			}
			std::cout << "PostSend Complete!!!" << std::endl;

			delete[] pSendData;
			
	});
}

void Session::Receive()
{
	if(m_IsPostReceive)
	{
		return;
	}
	else
	{
		m_IsPostReceive = true;
	}

	boost::asio::spawn(m_Socket.get_io_service(),
			[this](boost::asio::yield_context yield)
		{
			if (m_Socket.is_open() == false)
			{
				return;
			}

			boost::system::error_code ec;
			std::size_t bytes_transferred = m_Socket.async_read_some(boost::asio::buffer(m_ReceiveBuffer), yield[ec]);

			if(ec.value() > 0)
			{
				if( ec == boost::asio::error::eof )
				{
					std::cout << "클라이언트와 연결이 끊어졌습니다" << std::endl;
				}
				else if( ec.value() == 10054 )
				{
					std::cout << "클라이언트가 연결을 끊었습니다." << std::endl;
				}
				else 
				{
					std::cout << "error No: " << ec.value() << " error Message: " << ec.message() << std::endl;
				}

				m_pServer->CloseSession( m_nSessionID );
			}
			else
			{
				memcpy( &m_PacketBuffer[ m_nPacketBufferMark ], m_ReceiveBuffer.data(), bytes_transferred );
		
				int nPacketData = m_nPacketBufferMark + bytes_transferred;
				int nReadData = 0;
		
				while( nPacketData > 0 )
				{
					if( nPacketData < sizeof(PACKET_HEADER) ) 
					{
						break;
					}

					PACKET_HEADER* pHeader = (PACKET_HEADER*)&m_PacketBuffer[nReadData];
			
					if( pHeader->nSize <= nPacketData )
					{
						m_pServer->ProcessPacket( m_nSessionID, &m_PacketBuffer[nReadData] );
				
						nPacketData -= pHeader->nSize;
						nReadData += pHeader->nSize;
					}
					else
					{
						break;
					}
				}

				if( nPacketData > 0 )
				{
					char TempBuffer[MAX_RECEIVE_BUFFER_LEN] = {0,};
					memcpy( &TempBuffer[ 0 ], &m_PacketBuffer[nReadData], nPacketData );
					memcpy( &m_PacketBuffer[ 0 ], &TempBuffer[0], nPacketData );
				}

				m_nPacketBufferMark = nPacketData;
		  }
          
	});

	m_IsPostReceive = false;
}