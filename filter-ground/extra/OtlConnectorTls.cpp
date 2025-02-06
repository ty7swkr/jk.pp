/*
 * OtlConnectorTls.cpp
 *
 *  Created on: 2023. 12. 5.
 *      Author: tys
 */
#include "OtlConnectorTls.h"
#include "ScopeExit.h"
#include "Toggle.h"

// 쓰레드 로컬 스토리지 정의
// key: this 포인터, value: 쓰레드별 커넥터 원자적 포인터
//thread_local std::map<const void *, OtlConnectorTls::AtomicConnSptr> OtlConnectorTls::conn_tls_t::storage;

OtlConnectorTls::OtlConnectorTls()
{
  static std::once_flag __otl_once_flag__;
  std::call_once(__otl_once_flag__, []()
  {
    otl_connect::otl_initialize(1);
  });
};

OtlConnectorTls::~OtlConnectorTls()
{
}

/**
 * @brief 커넥터 관리 쓰레드 시작
 * @return 쓰레드 시작 성공 여부
 */
bool
OtlConnectorTls::start()
{
  return BlockingDequeThread<>::start();
}

/**
 * @brief 커넥터 관리 쓰레드 중지
 * @return 쓰레드 중지 성공 여부
 */
bool
OtlConnectorTls::stop()
{
  if (BlockingDequeThread<>::stop() == false)
    return false;

  return true;
}

/**
 * @brief 현재 연결의 유효성을 테스트
 * @param err_func 에러 발생 시 호출될 콜백 함수
 * @return 연결 테스트 성공 여부
 * @details 간단한 SELECT 쿼리를 실행하여 연결 상태를 확인
 */
bool
OtlConnectorTls::test_connection(std::function<void(const otl_exception &e)> err_func)
{
  try
  {
    auto conn_sptr  = this->get_connector();
    if (conn_sptr == nullptr)
      return false;

    auto &conn      = *(conn_sptr.get());
    otl_stream selector(1, "SELECT 1 FROM DUAL", conn);
    while (!selector.eof())
    {
      int dummy;
      selector >> dummy;
    }
  }
  catch (otl_exception &e)
  {
    if (err_func != nullptr) err_func(e);
    return false;
  }

  return true;
}

/**
 * @brief 특정 커넥터의 연결 상태 테스트
 * @param conn_sptr 테스트할 커넥터
 * @return 연결 테스트 성공 여부
 */
bool
OtlConnectorTls::test(otl_conn_sptr conn_sptr)
{
  if (conn_sptr == nullptr)
    return false;

  try
  {
    auto &conn = *(conn_sptr.get());
    otl_stream selector(1, "SELECT 1 FROM DUAL", conn);
    while (!selector.eof())
    {
      int dummy;
      selector >> dummy;
    }
  }
  catch (const otl_exception &e)
  {
    (void)e;
    return false;
  }
  return true;
}

/**
 * @brief Oracle 새 연결 생성
 * @param 접속 스트링
 * @return 생성된 커넥터 공유 포인터
 */
otl_conn_sptr
OtlConnectorTls::connect(const std::string &rlogon_str)
{
  otl_conn_sptr conn = std::make_shared<otl_connect>();
  conn->rlogon(rlogon_str.c_str());
  return conn;
}

/**
 * @brief 현재 쓰레드의 커넥터 획득 또는 생성
 * @return 유효한 DB 커넥터
 * @throws otl_exception 연결 실패 시
 * @details
 * 1. 쓰레드 로컬 스토리지에서 커넥터 검색
 * 2. 없으면 새로 생성
 * 3. 있지만 연결이 끊어진 경우 재연결 시도
 * 4. 실패 시 재연결 대기열에 등록하고 예외 발생
 */
otl_conn_sptr
OtlConnectorTls::get_connector()
{
  // 현재 쓰레드의 첫 접근인 경우
  if (conn_tls_.has() == false)
  {
    // 일단 깡통으로 등록
    conn_tls_.set(std::make_shared<AtomicConn>());
    try
    {
      otl_conn_sptr conn = OtlConnectorTls::connect(rlogon_str_.load());
      conn_tls_.set(std::make_shared<AtomicConn>(conn));
//      ++connection_count_;
      return conn;
    }
    catch (const otl_exception &e)
    {
      // 연결 실패 시 재연결 대기열에 일단 깡통으로 등록
      disconns_.add(conn_tls_.get());
      waiter_.push_back(1);
      throw;
    }
  }

  // 기존 커넥터 획득
  otl_conn_sptr conn = conn_tls_.get()->load();
//  if (conn == nullptr)
//    throw sql::SQLException("Unable to connect to data source", "08001", 2002);
  if (conn == nullptr)
  {
    try
    {
      conn_tls_.get()->store(nullptr);
      conn = OtlConnectorTls::connect(rlogon_str_.load());
      conn_tls_.get()->store(conn);
//      ++connection_count_;
    }
    catch (const otl_exception &e)
    {
      disconns_.add(conn_tls_.get());
      waiter_.push_back(1);
      throw;
    }
    return conn;
  }

  // 커넥터 유효성 검사 및 필요시 재연결
  if (OtlConnectorTls::test(conn) == false)
  {
//    --connection_count_;
    try
    {
      conn = OtlConnectorTls::connect(rlogon_str_.load());
//      ++connection_count_;
      conn_tls_.get()->store(conn);
    }
    catch (const otl_exception &e)
    {
      disconns_.add(conn_tls_.get());
      waiter_.push_back(1);
      throw;
    }
  }

  return conn;
}

/**
 * @brief 연결 관리 쓰레드의 메인 루프 함수
 * @details 이 함수는 다음과 같은 작업을 수행합니다:
 * 1. 연결 해제된 커넥터들을 주기적으로 모니터링
 * 2. 연결 정보 갱신이 필요한 경우 reset_connection_info 콜백을 통해 갱신
 * 3. 각 끊어진 커넥터에 대해 재연결 시도
 * 4. 연결 실패 시 occur_connect_error 콜백을 통해 에러 처리
 * 5. 모든 연결이 복구되면 clear_connect_error 콜백을 통해 정상화 알림
 *
 * @note 이 함수는 BlockingDequeThread에서 상속받은 run() 메서드를 구현합니다.
 * @note waiter_.pop()을 통해 1초마다 깨어나 연결 상태를 체크합니다.
 * @note Toggle 클래스를 사용하여 에러 상태 변화를 추적합니다.
 */
void
OtlConnectorTls::run()
{
  // 에러 상태 토글 초기화 (초기값: false, 이전값: false)
  Toggle err_toggle(false, false);

  // 연결 해제된 커넥터들을 저장할 로컬 변수
  decltype(disconns_.conns) disconns;

  // 대기열에서 사용할 더미 데이터 (실제 값은 중요하지 않음)
  std::deque<int> dummy;

  // 메인 루프: 1초(1000ms)마다 깨어나서 연결 상태 체크
  // waiter_.pop()이 음수를 반환하면 쓰레드 종료
  while (waiter_.pop(dummy, 1000) >= 0)
  {
    // 스레드 안전하게 현재 연결 해제된 커넥터 목록을 가져옴
    disconns = disconns_.load();

    // 연결 해제된 커넥터가 있고 갱신 콜백이 설정된 경우
    // 연결 정보 갱신 (예: 새로운 접속 정보로 업데이트)
    if (disconns.size() > 0)
      if (reset_connection_info != nullptr)
         reset_connection_info();

    // 이번 주기에 처리된 연결 수를 추적
    size_t connected = 0;

    // 각 연결 해제된 커넥터에 대해 재연결 시도
    for (auto &atomic_conn : disconns)
    {
      ++connected;  // 처리 시도 카운트 증가
      err_toggle.turn_on();  // 에러 상태 활성화

      try
      {
        // Oracle 데이터베이스 재연결 시도
        otl_conn_sptr new_conn_sptr = OtlConnectorTls::connect(rlogon_str_.load());
        if (new_conn_sptr == nullptr)
          continue;  // 연결 실패 시 다음 커넥터로 진행

        // 새로운 연결 객체를 atomic하게 저장
        atomic_conn->store(new_conn_sptr);

        // 재연결된 커넥터를 목록에서 제거
        disconns_.del(atomic_conn);
      }
      catch (const otl_exception &e)
      {
        // 연결 실패 시 에러 콜백 호출 (설정된 경우)
        if (occur_connect_error != nullptr)
          occur_connect_error(e);
      }
    }

    // 이번 주기에 처리할 연결이 없고 (connected == 0)
    // 에러 상태가 해제되면서 (err_toggle.turn_off() == true)
    // 정상화 콜백이 설정된 경우
    // => 모든 연결이 정상화되었음을 알림
    if (connected == 0)
      if (err_toggle.turn_off() == true)
        if (clear_connect_error != nullptr)
          clear_connect_error();
  }
}
