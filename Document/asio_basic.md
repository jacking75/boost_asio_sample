### 멤버 함수와 프리 함수
read, write, async_read, async_write 계는 멤버 함수와 프리 함수가 있다

#### 3개의 async_read/async_write
read/write 함수는 3개씩 존재한다.
- 보통
    - read(), async_read(), write(), async_write() 가 해당된다. 표준적인 read/write를 한다.
- xxx_at
    - read_at(), async_read_at(), write_at(), async_write_at()이 해당된다. read/write를 할 바이트 수를 지정한다. 대량의 데이터를 조금씩 나누어서 처리하고 싶을 때에 사용하는 듯.
- xxx_until
    - read_until(), async_read_until(), write_until(), async_write_until()이 해당된다. 지정한 문자열이 출현할 때까지 read/write를 실시한다. 정규 표현 가능. 예를 들어 개행 코드마다 read/write 할 경우에 편리.