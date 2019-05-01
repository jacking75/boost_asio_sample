#pragma once
#include <SDKDDKVer.h>

#include <deque>

#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include "Protocol.h"


class ChatServer;

class Session 
{
public:
	Session(int nSessionID, boost::asio::io_context& io_service, ChatServer* pServer);
	~Session();

	void SetName( const char* pszName ) { m_Name = pszName; }
	const char* GetName()				{ return m_Name.c_str(); }

	int SessionID() { return m_nSessionID; }

	boost::asio::ip::tcp::socket& Socket() { return m_Socket; }

	void Init();
	
	void Receive();
	
	void Send(const int nSize, char* pData);
	
	
private:

	int m_nSessionID;
	boost::asio::ip::tcp::socket m_Socket;
	
	std::array<char, MAX_RECEIVE_BUFFER_LEN> m_ReceiveBuffer;

	int m_nPacketBufferMark;
	char m_PacketBuffer[MAX_RECEIVE_BUFFER_LEN*2];

	bool m_bCompletedWrite;

	std::deque< char* > m_SendDataQueue;

	std::string m_Name;

	ChatServer* m_pServer;

	bool m_IsPostReceive;
};