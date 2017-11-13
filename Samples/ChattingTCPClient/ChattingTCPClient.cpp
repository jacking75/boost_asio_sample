#include <SDKDDKVer.h>
#include <deque>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "..\ChattingTCPServer\Protocol.h"



class ChatClient
{
public:
	ChatClient(boost::asio::io_service& io_service)
    : m_IOService(io_service),
	  m_Socket(io_service)
	{
		m_bIsLogin = false;
		InitializeCriticalSectionAndSpinCount(&m_lock, 4000);
	}

	~ChatClient()
	{
		EnterCriticalSection(&m_lock);

		while( m_SendDataQueue.empty() == false )
		{
			delete[] m_SendDataQueue.front();
			m_SendDataQueue.pop_front();
		}

		LeaveCriticalSection(&m_lock);

		DeleteCriticalSection(&m_lock); 
	}
	
	bool IsConnecting() { return m_Socket.is_open(); }

	void LoginOK() { m_bIsLogin = true; }
	bool IsLogin() { return m_bIsLogin; }

	void Connect( boost::asio::ip::tcp::endpoint endpoint )
	{
		m_nPacketBufferMark = 0;

		m_Socket.async_connect( endpoint,
								boost::bind(&ChatClient::handle_connect, this,
									boost::asio::placeholders::error)
					);
	}

	void Close()
	{
		if( m_Socket.is_open() )
		{
			m_Socket.close();
		}
	}

	void PostSend( const bool bImmediately, const int nSize, char* pData )
	{
		char* pSendData = nullptr;

		EnterCriticalSection(&m_lock);		// �� ����
		
		if( bImmediately == false )
		{
			pSendData = new char[nSize];
			memcpy( pSendData, pData, nSize);
			
			m_SendDataQueue.push_back( pSendData );
		}
		else
		{
			pSendData = pData;
		}

		if( bImmediately || m_SendDataQueue.size() < 2 )
		{
			boost::asio::async_write( m_Socket, boost::asio::buffer( pSendData, nSize ),
									 boost::bind( &ChatClient::handle_write, this,
										boost::asio::placeholders::error,
										boost::asio::placeholders::bytes_transferred )
									);
		}

		LeaveCriticalSection(&m_lock);		// �� �Ϸ�
	}

	

private:

	void PostReceive()
	{
		memset( &m_ReceiveBuffer, '\0', sizeof(m_ReceiveBuffer) );

		m_Socket.async_read_some
					( 
						boost::asio::buffer(m_ReceiveBuffer), 
						boost::bind( &ChatClient::handle_receive, this, 
													boost::asio::placeholders::error, 
													boost::asio::placeholders::bytes_transferred ) 
				
					);
	}

	void handle_connect(const boost::system::error_code& error)
	{
		if (!error)
		{	
			std::cout << "���� ���� ����" << std::endl;
			std::cout << "�̸��� �Է��ϼ���!!" << std::endl;

			PostReceive();
		}
		else
		{
			std::cout << "���� ���� ����. error No: " << error.value() << " error Message: " << error.message() << std::endl;
		}
	}

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred)
	{
		EnterCriticalSection(&m_lock);			// �� ����

		delete[] m_SendDataQueue.front();
		m_SendDataQueue.pop_front();

		char* pData = nullptr;

		if( m_SendDataQueue.empty() == false )
		{
			pData = m_SendDataQueue.front();
		}
		
		LeaveCriticalSection(&m_lock);			// �� �Ϸ�

		
		if( pData != nullptr )
		{
			PACKET_HEADER* pHeader = (PACKET_HEADER*)pData;
			PostSend( true, pHeader->nSize, pData );
		}
	}

	void handle_receive( const boost::system::error_code& error, size_t bytes_transferred )
	{
		if( error )
		{
			if( error == boost::asio::error::eof )
			{
				std::cout << "Ŭ���̾�Ʈ�� ������ ���������ϴ�" << std::endl;
			}
			else 
			{
				std::cout << "error No: " << error.value() << " error Message: " << error.message() << std::endl;
			}

			Close();
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
					ProcessPacket( &m_PacketBuffer[nReadData] );
				
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


			PostReceive(); 
		}
	}

	void ProcessPacket( const char*pData )
	{
		PACKET_HEADER* pheader = (PACKET_HEADER*)pData;

		switch( pheader->nID )
		{
		case RES_IN:
			{
				PKT_RES_IN* pPacket = (PKT_RES_IN*)pData;
				
				LoginOK();
				
				std::cout << "Ŭ���̾�Ʈ �α��� ���� ?: " << pPacket->bIsSuccess << std::endl; 
			}
			break;
		case NOTICE_CHAT:
			{
				PKT_NOTICE_CHAT* pPacket = (PKT_NOTICE_CHAT*)pData;

				std::cout << pPacket->szName << ": " << pPacket->szMessage << std::endl; 
			}
			break;
		}
	}
 
	  

private:
	boost::asio::io_service& m_IOService;
	boost::asio::ip::tcp::socket m_Socket;

	std::array<char, 512> m_ReceiveBuffer;
	
	int m_nPacketBufferMark;
	char m_PacketBuffer[MAX_RECEIVE_BUFFER_LEN*2];

	CRITICAL_SECTION m_lock;
	std::deque< char* > m_SendDataQueue;

	bool m_bIsLogin;
};


int main()
{
	boost::asio::io_service io_service;

	auto endpoint = boost::asio::ip::tcp::endpoint( 
						boost::asio::ip::address::from_string("127.0.0.1"), 
						PORT_NUMBER);

    ChatClient Cliet( io_service );
	Cliet.Connect( endpoint );

    boost::thread thread( boost::bind(&boost::asio::io_service::run, &io_service) );
		

	char szInputMessage[MAX_MESSAGE_LEN * 2] = {0,};
    
	while( std::cin.getline( szInputMessage, MAX_MESSAGE_LEN) )
    {
		if( strnlen_s( szInputMessage, MAX_MESSAGE_LEN ) == 0 )
		{
			break;
		}

		if( Cliet.IsConnecting() == false )
		{
			std::cout << "������ ������� �ʾҽ��ϴ�" << std::endl;
			continue;
		}

		if( Cliet.IsLogin() == false )
		{
			PKT_REQ_IN SendPkt;
			SendPkt.Init();
			strncpy_s( SendPkt.szName, MAX_NAME_LEN, szInputMessage, MAX_NAME_LEN-1 );

			Cliet.PostSend( false, SendPkt.nSize, (char*)&SendPkt );
		}
		else
		{
			PKT_REQ_CHAT SendPkt;
			SendPkt.Init();
			strncpy_s( SendPkt.szMessage, MAX_MESSAGE_LEN, szInputMessage, MAX_MESSAGE_LEN-1 );

			Cliet.PostSend( false, SendPkt.nSize, (char*)&SendPkt );
		}
	}

	io_service.stop();

    Cliet.Close();
	
    thread.join();
  
	std::cout << "Ŭ���̾�Ʈ�� ������ �ּ���" << std::endl;

	return 0;
}