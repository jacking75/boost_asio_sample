#pragma once

namespace AsioHttp
{

struct Response
{
	bool			bSuccess;
	std::wstring	strData;
	unsigned int	statusCode;
};

// http://www.boost.org/doc/libs/1_48_0/doc/html/boost_asio/example/http/client/sync_client.cpp

class Client
{
public:
	Client(void);
	virtual ~Client(void);

	bool Init( const std::string& strServer, unsigned int codePage );
	const Response Request( const std::string& strPath );

protected:
	class ClientImpl;
	ClientImpl* m_pImpl;
};

std::string ToUrlString( const std::wstring& src );
std::string ToUrlString( const std::string& src );

} // namespace AsioHttp