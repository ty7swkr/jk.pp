/*
 * MariaConnectorTls.cpp
 *
 *  Created on: 2023. 12. 5.
 *      Author: tys
 */
#include "MariaResultSet.h"
#include "MariaStatement.h"
#include "ScopeExit.h"
#include "Toggle.h"
#include <Logger.h>

// 쓰레드 로컬 스토리지 정의
// key: this 포인터, value: 쓰레드별 커넥터 원자적 포인터
//thread_local std::map<const void *, MariaConnectorTls::AtomicConnSptr> MariaConnectorTls::conn_tls_t::storage;

MariaConnectorTls::~MariaConnectorTls()
{
}

/**
 * @brief 커넥터 관리 쓰레드 시작
 * @return 쓰레드 시작 성공 여부
 */
bool
MariaConnectorTls::start()
{
  return BlockingDequeThread<>::start();
}

/**
 * @brief 커넥터 관리 쓰레드 중지
 * @return 쓰레드 중지 성공 여부
 */
bool
MariaConnectorTls::stop()
{
  if (BlockingDequeThread<>::stop() == false)
    return false;

  disconns_.conns.clear();
  conn_tls_.clear();
  return true;
}

/**
 * @brief 현재 연결의 유효성을 테스트
 * @param err_func 에러 발생 시 호출될 콜백 함수
 * @return 연결 테스트 성공 여부
 * @details 간단한 SELECT 쿼리를 실행하여 연결 상태를 확인
 */
bool
MariaConnectorTls::test_connection(std::function<void(sql::SQLException &e)> err_func)
{
  try
  {
    auto conn = this->get_connector();
    if (conn == nullptr)
      return false;

    MariaStatement stmt(*this, "select 1");
    MariaResultSet rs = stmt.execute_query();
    int32_t dummy;
    while (rs.next() == true)
      rs >> dummy;
  }
  catch (sql::SQLException &e)
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
MariaConnectorTls::test(maria_conn_sptr conn_sptr)
{
  try
  {
    std::unique_ptr<sql::PreparedStatement> pstmt (conn_sptr->prepareStatement("SELECT 1"));
    std::unique_ptr<sql::ResultSet>         rs    (pstmt->executeQuery());
  }
  catch (const sql::SQLException &e)
  {
    (void)e;
    return false;
  }
  return true;
}

/**
 * @brief MariaDB 새 연결 생성
 * @param url DB 연결 URL
 * @param properties 연결 속성
 * @return 생성된 커넥터 공유 포인터
 */
maria_conn_sptr
MariaConnectorTls::connect(const sql::SQLString  &url,
                           const sql::Properties &properties)
{
//  // 커스텀 삭제자(deleter) 함수를 만들어요
//  // 나중에 디버깅용으로 남겨놓음.
//  struct MariaConnDeleter
//  {
//    void operator()(sql::Connection *conn)
//    {
//      if (conn == nullptr) return;
//      conn->close();
//
//      sfs_log.warn() << "close mariadb conn";
//      delete conn;
//    }
//  };
  // return maria_conn_sptr(sql::mariadb::get_driver_instance()->connect(url, properties), MariaConnDeleter());
  return maria_conn_sptr(sql::mariadb::get_driver_instance()->connect(url, properties));
}

/**
 * @brief 현재 쓰레드의 커넥터 획득 또는 생성
 * @return 유효한 DB 커넥터
 * @throws sql::SQLException 연결 실패 시
 * @details
 * 1. 쓰레드 로컬 스토리지에서 커넥터 검색
 * 2. 없으면 새로 생성
 * 3. 있지만 연결이 끊어진 경우 재연결 시도
 * 4. 실패 시 재연결 대기열에 등록하고 예외 발생
 */
maria_conn_sptr
MariaConnectorTls::get_connector()
{
  // 현재 쓰레드의 첫 접근인 경우
  if (conn_tls_.has() == false)
  {
    // 일단 깡통으로 등록
    conn_tls_.set(std::make_shared<AtomicConn>());
    try
    {
      maria_conn_sptr conn = MariaConnectorTls::connect(url_.load(), properties_.load());
      conn_tls_.get()->store(conn);
//      ++connection_count_;
      return conn;
    }
    catch (const sql::SQLException &e)
    {
      // 연결 실패 시 재연결 대기열에 일단 깡통으로 등록
      disconns_.add(conn_tls_.get());
      waiter_.push_back(1);
      throw;
    }
  }

  // 기존 커넥터 획득
  maria_conn_sptr conn = conn_tls_.get()->load();
//  if (conn == nullptr)
//    throw sql::SQLException("Unable to connect to data source", "08001", 2002);
  if (conn == nullptr)
  {
    try
    {
      conn = MariaConnectorTls::connect(url_.load(), this->properties_.load());
      conn_tls_.get()->store(conn);
//      ++connection_count_;
    }
    catch (const sql::SQLException &e)
    {
      disconns_.add(conn_tls_.get());
      waiter_.push_back(1);
      throw;
    }
    return conn;
  }

  // 커넥터 유효성 검사 및 필요시 재연결
  if (MariaConnectorTls::test(conn) == false)
  {
//    --connection_count_;
    try
    {
      conn_tls_.get()->store(nullptr);
      conn = MariaConnectorTls::connect(url_.load(), this->properties_.load());
      conn_tls_.get()->store(conn);
//      ++connection_count_;
    }
    catch (const sql::SQLException &e)
    {
      disconns_.add(conn_tls_.get());
      waiter_.push_back(1);
      throw;
    }
  }

  return conn;
}

/**
 * @brief 연결 관리 쓰레드 메인 루프
 * @details
 * 1. 주기적으로 연결 해제된 커넥터들을 확인
 * 2. 재연결 시도
 * 3. 연결 상태에 따른 콜백 함수 호출
 * 4. 연결 정보 갱신 필요시 갱신
 */
void
MariaConnectorTls::run()
{
  // 에러 상태를 추적하기 위한 토글 객체 초기화
  Toggle err_toggle(false, false);
  // 연결 해제된 커넥터들을 저장할 로컬 컨테이너
  decltype(disconns_.conns) disconns;

  // 1초(1000ms) 간격으로 연결 상태 체크 루프 실행
  std::deque<int> dummy;
  while (waiter_.pop(dummy, 1000) >= 0)
  {
    // 현재 연결 해제된 커넥터들의 목록을 로컬 변수로 복사
    disconns = disconns_.load();

    // 연결 해제된 커넥터가 있는 경우, 연결 정보 갱신 콜백 실행
    if (disconns.size() > 0)
      if (reset_connection_info != nullptr)
         reset_connection_info();

    // 재연결된 커넥터 수 추적
    size_t connected = 0;
    // 연결 해제된 각 커넥터에 대해 재연결 시도
    for (auto &atomic_conn : disconns)
    {
      ++connected;
      // 에러 상태 활성화
      err_toggle.turn_on();

      try
      {
        // MariaDB 서버에 새로운 연결 시도
        auto new_conn_sptr = MariaConnectorTls::connect(url_.load(), this->properties_.load());
        if (new_conn_sptr == nullptr)
          continue;

        // 새로운 연결을 atomic 커넥터에 저장하고 연결 해제 목록에서 제거
        atomic_conn->store(new_conn_sptr);
//        ++connection_count_;
        disconns_.del(atomic_conn);
      }
      catch (const sql::SQLException &e)
      {
        // 연결 실패 시 에러 콜백 호출
        if (occur_connect_error != nullptr)
          occur_connect_error(const_cast<sql::SQLException &>(e));
      }
    }

    // 모든 커넥터가 정상적으로 연결된 경우 에러 클리어 콜백 호출
    if (connected == 0)
      if (err_toggle.turn_off() == true)
        if (clear_connect_error != nullptr)
          clear_connect_error();
  }
}
