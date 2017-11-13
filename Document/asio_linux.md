### boost 빌드 및 설정
boost 라이브러리를 다운로드 후 압축을 푼다.
<pre>
$cd BOOST의 루트
$./bootstrap.sh
$./b2 install-prefix=라이브러리 패스
</pre>
이것으로 끝이다.  

-prefix를 생략하면 /usr/local/lib 아래에 만들어진다.

#### 빌드
예제 코드
```
#include <iostream>
#include <boost/asio.hpp>

using namespace std;
using namespace boost::asio ;

static const int PORT_NO = 8080;

int main() {

    boost::asio::io_service     io;


    while(true) {

        ip::tcp::iostream           buf;
        ip::tcp::acceptor           accept( io, ip::tcp::endpoint(ip::tcp::v4(), PORT_NO ));
        accept.accept(*buf.rdbuf());

        std::string temp;

        while (true) {
            buf >> temp;
            if (temp == "quit") {
                buf << ">>bye" << std::endl;
                break;
            }
            buf << ">>" << temp << std::endl;
        }
    }

    return 0;
}
```
  
cmake 파일
```
cmake_minimum_required(VERSION 2.8.4)
project(asio1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_HOME_DIRECTORY}/bin")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)



find_package(Boost 1.57.0 COMPONENTS system)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(asio1 ${SOURCE_FILES})
    target_link_libraries(asio1 ${Boost_LIBRARIES})
endif()
```
  
  
  