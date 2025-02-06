  
### helper

여러가지 스트링 관련 유틸리티성 함수들

trim, upper, lower, extract(스트링), split, 안전한 substr, compare_front(앞문자열 비교), compare_rear(뒷 문자열 비교)

replace_all, concat_with_delimiter 등.


### AtomicSptr

thread-safe std::atomic은 std::shared_ptr지원을 어렵게 해서 랩핑한 클래스
  

### LockedObject

thread-safe 복사 불가능한 타입을 잠금상태에서 다루기 위한 구현체

LockedObject\<std::string\> a; a = "123"; a.load() or a->length();

  

### BlockingDeque

thread-safe 양방향 Blocking 구현체. 큐가 비어있있을 때 스레드 blocking(pop)

  

### BlockingDequeThread

thread-safe BlockingDeque 쓰레드 구현체

  

### BlockingLockFreeQueue

thread-safe Boost lockfree Queue 를 이용한 Blocking Queue

  

### BlockingVector

thread-safe std::vector를 기반으로 하는 Blocking 큐 구현체. 큐가 비어있있을 때 스레드 blocking(pop)

  

### BlockingVectorThread

thread-safe BlockingDeque 쓰레드 구현체

  

### LockFreeQueueThread

thread-safe Boost lockfree Queue 를 이용한 쓰레드 구현체

  

### LockedDeque

thread-safe std::deque를 thread-safe 하게 다루기 위한 구현체
  
### MJsonObject
Managed Json: json reader로 json 접근을 쉽게 하기 위한 rapidjson 랩퍼
MJsonObject obj = conf["name"][0][1]["subject"].as_str() ...

### MSignal

thread-safe std::condition_variable를 클래스로 랩핑 및 시그널유실 방지 기능 추가

### MThread

thread-safe std::thread를 Java 쓰레드 인터페이스와 유사한 스타일의 구현체

  

### MariaDB Connector cpp

템플릿을 이용하여 타입 자동 추론등의 기능을 추가한 구현체.

MariaStatement - select/delete/insert/update 구문처리 랩핑 클래스

MariaCallableStatement - 프로시져용 랩핑 클래스

MariaConnectorTls - thread local storage 기반 커넥터, 연결 실패 시 자동 재연결기능등

MariaResultSet - 쿼리 결과 저장 클래스

MariaResultVal - 결과 값을 저장하고 다루기 위한 구현체


### OtlConnectorTls

오라클용 otl 라이브러리를 이용하여 thread local storage기반의 otl_connect 자동 관리 구현체 


### Optional

c++17의 std::optional 기능을 사용하기 위해 c++11용으로 구현한 optional


### ScopeExit

블럭 종료시 자동 호출 해주는 클래스

SCOPE_EXIT({ std::cout << "return function" << std::endl});

  

### Singleton

thread-safe 싱글턴 패턴

  

### SpinLock

thread-safe 스핀락

  

### SpinLockGuard

thread-safe 스핀락 가드 (std::lock_guard의 SpinLock버전)


### 날짜 시간

SysTime - 현재 시간 설정, 가감등

SysDate - 현재 날짜 설정, 가감등

SysDateTime - 시간날짜 한번에 다룰 수 있는 구현체. 여러 형태의 시간값으로 변환등(time_t, timeval, timespec등...)

SysDateTimeDiff - SysDateTime의 시간차 계산 구현체

- ex)

auto now = SysDateTime::now()

SysDateTimeDiff diff = now - (now-SysDate::Day(1));

std::cout << diff.seconds; << std::endl // 86400

  

### TpsMeter

thread-safe TPS측정 클래스

  

### Toggle

thread-safe 토글 스위치를 구현한 구현채. 플래그 설정과 비슷.
