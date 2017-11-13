#include <SDKDDKVer.h>

#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>


void OnReadFile( const boost::system::error_code& error, const std::vector<char>& result )
{
	if (error)
	{
		std::cout << "���� �б� ����: " << error.message() << std::endl;
	}
	else 
	{
		std::cout << "���� �б� ����" << std::endl;
		
		const std::string s(result.begin(), result.end());
		std::cout << s << std::endl;
	}
}


int main()
{
	boost::asio::io_service io_service;

	HANDLE handle = ::CreateFileA( "AsyncReadFile.txt",
									GENERIC_READ,
									FILE_SHARE_READ,
									NULL,
									OPEN_EXISTING,
									FILE_FLAG_OVERLAPPED,
									NULL );

	if (handle == INVALID_HANDLE_VALUE) 
	{
		std::cout << "���� ���� ����. " << "AsyncReadFile.txt" << std::endl;
		return 0;
	}

	std::vector<char> buffer(::GetFileSize(handle, NULL));

	boost::asio::windows::stream_handle file(io_service, handle);

	boost::asio::async_read( file, 
							boost::asio::buffer(buffer.data(), buffer.size()),
							boost::bind( OnReadFile, 
										boost::asio::placeholders::error, 
										boost::ref(buffer) )
							);

	io_service.run();

	getchar();
	::CloseHandle(handle);
	return 0;
}
