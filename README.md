중요한 정보등은 제거하여 컴파일은 되지 않습니다.

## filter-ground

- [Worker vs FilterWorker](#worker-vs-filterworker)
- [Worker 사용 가이드](#worker-사용-가이드)
- [FilterWorker 사용 가이드](#FilterWorker-사용-가이드)

### 개요
- 멀티쓰레드 기반 boost:lockfree:queue를 사용한 Producer-Consumer Framework
- 모든 동작은 thread-safe(atomic, mutex, spinlock, CAS(Compare And Swap))

### 주요 특징
- [SOLID원칙](https://ko.wikipedia.org/wiki/SOLID_(%EA%B0%9D%EC%B2%B4_%EC%A7%80%ED%96%A5_%EC%84%A4%EA%B3%84))을 준수하여 각 클래스의 책임과 역할을 분리
- [RAII 패턴](https://blog.heycoach.in/raii-resource-acquisition-is-initialization-in-c/)을 사용한 안전한 리소스 관리
- 템플릿 기반 타입 안정성 컴파일 검증
- Publisher/FilterWorker 수 조절을 통한 처리량 확장 가능
- Least Loaded 방식의 FilterWorker 작업 분배
- Lock-Free Queue 기반 동시성 처리

### 디렉토리
- extra: Oracle,MariaDB,Thread,Signal,LockFreeQueue등 유틸성 클래스 모음<br>
- filter_info: 필터링구조체 <-> json
- json: rapidjson helper
- oraspam: Oracle Table 관련 클래스
- package: 필요한 패키지
- thirdparty: rapidjson 라이브러리

### 개발 환경
- c++11
- Rocky Linux 9.4 (Blue Onyx)
  - g++ version 11.5.0 20240719 (Red Hat 11.5.0-2)
  - boost-1.75.0-8
- mariadb-connector-c-devel-3.2.6-1.el9_0.x86_64
- mariadb-connector-cpp-1.1.5-1.el9.x86_64
- natsc 클라이언트 라이브러리(3.10.0-beta)
- rapidjson(v1.1.0-blue)
- Make 빌드

### 기반 클래스
- MThread: std::thread 기본 동작을 담당하는 클래스 (Template Method - run() 가상함수)
- MSignal: 쓰레드 간 시그널링. std::condition_variable에서 spurious wakeup과 lost wakeup 방지 처리
- BlockingLockFreeQueue: boost::lockfree::queue + MSignal를 사용한 Blocking 기반 메세지 큐
- LockFreeQueueThread: BlockingLockFreeQueue(boost랩핑) + MThread 사용한 클래스로 쓰레드간 메세지 교환에 사용

### 메시지 발행(Publisher) 관련 클래스들
- NatsPublisher: LockFreeQueueThread를 상속받아 단일 NATS client.publish 쓰레드 구성
- NatsPublisherPool: 여러 NatsPublisher를 관리하여 라운드로빈 방식으로 메시지 발행

### 메시지 구독(Subscriber) 관련 클래스들
- NatsRecvers: NATS client.subscribe(subject)와 WorkerPool과 연동하여 메시지 처리
- WorkerPool: 여러 Worker를 관리하여 작업 분산
- Worker: 작업 처리 인터페이스
- FilterWorker: 필터에 필요한 기본처리 (AuthFilterWorker, SmishingFilterWorker...)

### 상속 관계
MThread<br>
 └── LockFreeQueueThread<br>
    ├── Worker<br>
    └── NatsPublisher<br>

### 포함 관계
- LockFreeQueueThread가 BlockingLockFreeQueue를 포함
- WorkerPool이 Worker들을 포함
- NatsPublisherPool이 NatsPublisher들을 포함
- NatsRecvers가 WorkerPool을 참조

### Database
- CnapsDB: thread-safe MariaDB 커넥터 관리 - 싱글턴
- OraSpamDB: thread-safe Oracle 커넥터 관리 - 싱글턴

## Worker vs FilterWorker

**SOLID원칙에 따라 아래와 같이 분리 작성되었으며, FilterWorker는 필터링 도메인 전용입니다.
다른 도메인 적용시에는 Worker를 상속받아 구현합니다.**


```
WorkerPool (Worker 관리)
   |
Worker (Base Class)
   |
   |-----> FilterWorker (필터 전용)
   |          |-----> AuthFilterWorker
   |          |-----> SmishingFilterWorker
   |          |-----> ...
   |
   |-----> xxxWorker (일반 Worker 상속)
```
- Worker & WorkerPool
  
  - Worker
    - 실제 작업을 처리하는 쓰레드 단위의 작업자 입니다.
    - Worker를 상속받으면 가상함수인 run()과 push()를 구현과 함께 필요한 요소들을 구현 합니다.
  - WorkerPool
    - 여러 Worker들을 관리하고 Least Loaded 방식으로 작업을 분배 합니다.
      
- FilterWorker

  - 필터링이라는 특정 도메인에 특화된 클래스로 AuthFilter, SmishingFilter처럼 필터링 목적으로만 사용되도록 구성된 클래스입니다.
  - discard tps in/out, 필터용 Nats 송수신, 필터용 AppConf등 필터링 목적에 맞게 구성되었습니다.
  - 인터페이스 분리 원칙(ISP)에 따라 필터링이라는 특정 도메인에 필요한 기능만을 제공합니다.
    
- 필터링 도메인 외 목적
  - 단일 책임 원칙(SRP) 혹은 인터페이스 분리 원칙(ISP) 따라 필터링 외 목적 혹은 인터페이스가 조금이라도 다른 작업이라면 Worker를 상속받아 구현해야 합니다.
  - 이 인터페이스를 필터링외 목적으로 수정하게 되는 경우 결합도가 올라게 됩니다. 따라서 기존 필터의 실행 안정성이 떨어지며, 유지보수가 어려울 수 있습니다.
  - 이와 비슷한 기능과 인터페이스라서 FilterWorker를 복사, 수정하더라도 다른 이름으로 대체해 사용하는 것이 안전합니다.(Wokrer베이스의 또 다른 Worker가 됩니다.)

  

## Worker 사용 가이드

- Worker 클래스 상속과 기본 구조
  - Worker는 템플릿 클래스로 작업할 데이터 타입을 지정해야 합니다.
  - LockFreeQueueThread를 상속받아 쓰레드 안전한 큐잉을 제공합니다.
  - 가상함수인 run()과 push()를 반드시 구현해야 합니다.

- run()과 push() 메서드 구현
  - run(): 큐에서 데이터를 꺼내 실제 작업을 처리하는 로직을 구현합니다.
  - push(): 작업 데이터를 큐에 추가하는 로직을 구현합니다.
  - try_push() 함수를 통해 재시도 로직을 구현할 수 있습니다.

- WorkerPool 연동
  - WorkerPool은 생성된 Worker들을 관리합니다.
  - 관리하고자 하는 Worker를 템플릿으로(FilterWorker, xxxWorker.....) 주입합니다.
  - set_num_of_workers()로 Worker 수와 큐 크기를 설정합니다.
  - Least Loaded 방식으로 자동으로 작업을 분배합니다.

- 구현시 주의사항
  - Worker는 단일 책임을 가지도록 구현해야 합니다.
  - 모든 멤버 변수 접근은 쓰레드에 안전하게 구현해야 합니다.
  - 큐잉, 워커수 등의 설정은 NatsRecver 클래스 참조

## FilterWorker 사용 가이드

- 사용 대상
  - 필터링 어플리케이션 개발시에만 사용합니다
  - 필터링 외 다른 목적이면 Worker를 상속받아 구현해야 합니다

- 의존 클래스: Worker클래스와의 차이점
  - Worker클래스는 아래 의존클래스가 없습니다.
  - AppConf: 필터 설정 관리
  - NatsSender/NatsResult: NATS 메시지 송수신
  - CnapsDB: MariaDB 연동
  - filter_info_t: NATS 송수신 메세지 규약을 구성한 구조체

- 필수 구현 사항
  - handle_filter() 메서드NATS를 통해 메세지가 수신되면 handle_filter가 호출됩니다.
    - 필터링 처리 결과에 따라:
      - to_next_nats(): 다음 필터로 전달
      - to_result_nats(): 결과 처리
    - AuthFilterWorker 참고

  - get_app_conf() 메서드 구현
  - AppConf 클래스
    - 필수 설정: system_id, db_configs, nats 관련 설정들
    - 사용자 설정: read() 메서드의 user_config 콜백을 통해 추가 가능
    - 설정파일 위치: CNAPS_CONFIG_PATH 환경변수

- 제공되는 기능들
  - discard 처리 (tps, timeout 등)
  - NATS 메시지 송수신
  - 필터 결과 처리
