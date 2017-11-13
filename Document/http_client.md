### boost.asio로 작성한 http client
- Boost.Asio 활용 방법 중 괜찮을 글이 있어서 원저자의 동의를 받고 소개한다.
- http://devnote.tistory.com/226

#### 기본 사용하기
```
nclude "HttpClient.h"

void main()
{
    std::wcout.imbue( std::locale("") );

    AsioHttp::Client client;
    client.Init( "leafbird.net", CP_UTF8 );

    std::wcout << client.Request( "/index.php" ).strData << std::endl;
}
```

#### 네이버 오픈 API 사용하기
```
#include "HttpClient.h"

void main()
{
    std::wcout.imbue( std::locale("") );

    AsioHttp::Client client;

    // call Naver OpenAPI
    client.Init( "openapi.naver.com", CP_UTF8 );

    boost::format fmt( "/search?key=%1%&query=%2%&display..." );
    fmt % "개인 개발자 등록키 값"
        % AsioHttp::ToUrlString( "안철수" );

    std::wcout << client.Request( fmt.str().c_str() ).strData;
}
```
[예제 파일](resource\HttpClient.zip)
