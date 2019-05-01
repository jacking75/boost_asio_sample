// http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/example/cpp11/futures/daytime_client.cpp
// 예제를 참고로 했음

#include <future>
#include <iostream>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/use_future.hpp>

using boost::asio::ip::udp;

const char UDP_IP[] = "127.0.0.1";
const unsigned short SERVER_PORT_NUMBER = 31400;
const unsigned short CLIENT_PORT_NUMBER = 31401;

void Echo(boost::asio::io_context& io_service )
{
	static int nSeqNumber = 0;
	
	try
	{
		++nSeqNumber;

		udp::socket socket(io_service, udp::endpoint(boost::asio::ip::udp::v4(), CLIENT_PORT_NUMBER));
		auto endpoint = udp::endpoint( boost::asio::ip::make_address(UDP_IP), SERVER_PORT_NUMBER);

		char szMessage[128] = {0,};
		sprintf_s( szMessage, 128-1, "%d - Send Message", nSeqNumber );

		std::future<std::size_t> send_length =
							  socket.async_send_to(boost::asio::buffer(szMessage),
								  endpoint, 
								  boost::asio::use_future);

		// 보내기가 완료될 때까지 대기한다.
		send_length.get(); // 

		
		std::array<char, 128> recv_buf;
		udp::endpoint sender_endpoint;
		std::future<std::size_t> recv_length =
						  socket.async_receive_from(
							  boost::asio::buffer(recv_buf),
							  sender_endpoint,
							  boost::asio::use_future);
				
		std::cout.write( recv_buf.data(),
						recv_length.get()); // 받기 완료 때까지 대기한다.
	}
	catch (std::system_error& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

int main()
{
	try
	{
		boost::asio::io_context io_service;
		boost::asio::io_context::work work(io_service);
		
		std::thread thread([&io_service](){ io_service.run(); });

		Echo(io_service);

		io_service.stop();
		thread.join();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	std:: cout << "네트웍 종료" << std::endl;

	getchar();
	return 0;
}