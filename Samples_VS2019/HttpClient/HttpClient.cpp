#include "StdAfx.h"
#include "HttpClient.h"

namespace AsioHttp
{

using boost::asio::ip::tcp;

class Client::ClientImpl
{
public:
	bool Init( const std::string& strServer, unsigned int codePage );
	const Response Request( const std::string& strPath );

protected:
	std::string		m_strServer;
	unsigned int	m_codePage;
};

std::string ToAnsiString( const std::wstring& src, UINT codePage )
{
	std::string strReturn;

	const int nLength = ::WideCharToMultiByte( codePage, 0UL, src.c_str(), -1, nullptr, 0, nullptr, nullptr );

	if( nLength < 0 )
		return strReturn;

	strReturn.resize( nLength );
	const int nResult = ::WideCharToMultiByte( codePage, 0UL, src.c_str(), -1, &strReturn[0], nLength, nullptr, nullptr );

	(void)nResult;
	return strReturn;
}

std::wstring ToWideString( const std::string& src, UINT codePage )
{
	std::wstring strReturn;

	const int nLength = ::MultiByteToWideChar( codePage, 0UL, src.c_str(), -1, nullptr, 0 );

	if( nLength < 0 )
		return strReturn;

	strReturn.resize( nLength );

	const int nResult = ::MultiByteToWideChar( codePage, 0UL, src.c_str(), -1, &strReturn[0], nLength );

	(void)nResult;

	return std::move(strReturn);
}

std::string UTFStringToUrlString( const std::string& src )
{
	std::stringstream stm;

	for each( char ch in src )
	{
		if( ch == 0 )	// 가장 마지막 널문자는 변환하지 않는다.
			break;

		stm << boost::format( "%%%X" ) % (ch & 0xff);
	}

	return stm.str();
}

std::string ToUrlString( const std::wstring& src )
{
	return UTFStringToUrlString( ToAnsiString( src, CP_UTF8 ) );
}

std::string ToUrlString( const std::string& src )
{
	return ToUrlString( ToWideString( src, CP_ACP ) );
}


bool Client::ClientImpl::Init( const std::string& strServer, unsigned int codePage )
{
	m_strServer = strServer;
	m_codePage = codePage;

	return true;
}

const Response Client::ClientImpl::Request( const std::string& strPath ) try
{
	Response ret;

	boost::asio::io_service io_service;

	// Get a list of endpoints corresponding to the server name.
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(m_strServer, "http");
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

	// Try each endpoint until we successfully establish a connection.
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);

	// Form the request. We specify the "Connection: close" header so that the
	// server will close the socket after transmitting the response. This will
	// allow us to treat all data up until the EOF as the content.
	boost::asio::streambuf request;
	std::ostream request_stream(&request);
	request_stream << "GET " << strPath << " HTTP/1.0\r\n";
	request_stream << "Host: " << m_strServer << "\r\n";
	request_stream << "Accept: */*\r\n";
	request_stream << "Connection: close\r\n\r\n";

	// Send the request.
	boost::asio::write(socket, request);

	// Read the response status line. The response streambuf will automatically
	// grow to accommodate the entire line. The growth may be limited by passing
	// a maximum size to the streambuf constructor.
	boost::asio::streambuf response;
	boost::asio::read_until(socket, response, "\r\n");

	// Check that response is OK.
	std::istream response_stream(&response);
	std::string http_version;
	response_stream >> http_version;
	//unsigned int status_code;
	response_stream >> ret.statusCode;
	std::string status_message;
	std::getline(response_stream, status_message);
	if (!response_stream || http_version.substr(0, 5) != "HTTP/")
	{
		ret.bSuccess = false;
		ret.strData = L"Invalid response\n";
		return ret;
	}
	if (ret.statusCode != 200)
	{
		ret.bSuccess = false;
		ret.strData = ( boost::wformat( L"Response returned with status code %s\n" ) % ret.statusCode ).str();
		return ret;
	}

	// Read the response headers, which are terminated by a blank line.
	boost::asio::read_until(socket, response, "\r\n\r\n");

	// Process the response headers.
	std::string header;
	while (std::getline(response_stream, header) && header != "\r")
		std::cout << header << "\n";
	std::cout << "\n";

	// Write whatever content we already have to output.
	std::stringstream stm;
	if (response.size() > 0)
		stm << &response;

	// Read until EOF, writing data to output as we go.
	boost::system::error_code error;
	while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
		stm << &response;
	if (error != boost::asio::error::eof)
		throw boost::system::system_error(error);

	ret.strData = ToWideString( stm.str(), m_codePage );

	return std::move( ret );
}
catch (std::exception& e)
{
	std::cout << "Exception: " << e.what() << "\n";

	Response ret = { false, std::wstring(), 0 };
	return std::move( ret );
}


////////////////////////////////////////////////////////////////////////////////

Client::Client(void) : m_pImpl( new ClientImpl )
{
}


Client::~Client(void)
{
	delete m_pImpl;
	m_pImpl = nullptr;
}

bool Client::Init( const std::string& strServer, unsigned int codePage )
{
	return m_pImpl->Init( strServer, codePage );
}


const Response Client::Request( const std::string& strPath )
{
	return m_pImpl->Request( strPath );
}


} // namespace AsioHttp