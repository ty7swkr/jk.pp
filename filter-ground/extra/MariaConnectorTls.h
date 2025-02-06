/*
 * MariaDBTLS.h
 *
 *  Created on: 2024. 11. 20.
 *      Author: tys
 */

#pragma once

#include <extra/BlockingDequeThread.h>
#include <extra/SpinLockGuard.h>
#include <extra/LockedObject.h>
#include <extra/AtomicSptr.h>
#include <extra/Singleton.h>
#include <extra/TlInstance.h>

#include <mariadb/conncpp.hpp>

#include <memory>
#include <set>

inline bool
operator==(const sql::Properties &props1, const sql::Properties &props2)
{
  for (const auto &prop : props1)
  {
    // iterator의 first는 key, second는 value
    auto it = props2.find(prop.first);
    if (it == props2.end() || it->second != prop.second)
      return false;
  }
  return true;
}

using maria_conn_sptr = std::shared_ptr<sql::Connection>;

/***
 * @brief MariaDB Connector 쓰레드 로컬 스토리지
 * 쓰레드별로 Connector를 저장하고 커넥트를 대행함.
 * 별도 쓰레드가 실행되어 끊어진 커넥터를 주기적으로 연결 시도함.
 * 종료시 모든것이 자동해제됨.
 * tls 사용한 것은 쓰레드 종료시 자동 해제됨.
 *
 * @see MariaStatement
 * @see MariaResultSet
 */

/**
 * @class MariaConnectorTls
 * @brief Thread Local Storage 기반 MariaDB 커넥터 관리 클래스.
 *
 * MariaDB와의 연결을 관리하고, 연결 상태를 모니터링하며,
 * 연결 실패 시 재연결을 시도하는 기능을 제공합니다.
 * 연결 상태에 따른 알람 콜백도 지원합니다.
 */
class MariaConnectorTls : protected BlockingDequeThread<>
{
public:
  MariaConnectorTls() {};
  virtual ~MariaConnectorTls();

  // 접속 실패 시 호출되는 알람 콜백 함수.
  std::function<void(sql::SQLException &e)> occur_connect_error = nullptr;

  // 모든 커넥터가 성공적으로 연결된 후 호출되는 콜백 함수.
  std::function<void()> clear_connect_error = nullptr;

  // connection info를 재접속시 변경하고 싶을때 사용하는 함수.
  // 재접속시 이 람다함수가 설정되어 있으면 호출하여 준다.
  /**
  {
    auto reloaded_db_config = app_conf.get_next_db_config();
    if (reloaded_db_config.empty == true)
      return;

    if (reloaded_db_config.url()        == cnaps_db.url() &&
        reloaded_db_config.properties() == cnaps_db.properties())
      return;

    cnaps_db.set_connection_info(reloaded_db_config.url(),
                                 reloaded_db_config.properties());
  };
   */
  std::function<void()> reset_connection_info = nullptr;

  /**
   * @brief MariaDB 접속 정보를 설정.
   * @param url 접속 URL.
   * @param properties 접속 프로퍼티.
   * url에 user/password포함시키기 : "jdbc:mariadb://1.1.1.8:3306/testdb?user=USER&password=PASSWORD"
   * 또는 나누어서 url = jdbc:mariadb://1.1.1.8:3306/testdb, properties = {{"user", "my"}, {"password", "123qwe"}}
   */
  void set_connection_info(const sql::SQLString   &url,
                           const sql::Properties  &properties = {})
  {
    url_        = url;
    properties_ = properties;
  }

  std::string     url       () const { return url_.load().c_str(); }
  sql::Properties properties() const { return properties_.load();  }

  /**
   * @brief MariaDB Connector 인스턴스를 반환.
   *
   * @details 필요 시 Test 쿼리를 실행하고 실패하면 재연결을 시도합니다.
   * 재연결 실패 시 연결 쓰레드에서 처리를 요청하고 예외를 던집니다.
   *
   * @return MariaDB 연결 객체(maria_conn_sptr).
   * @throw sql::SQLException 연결 실패 시 예외 발생.
   */
  maria_conn_sptr get_connector();

  /**
   * @brief 연결 상태 테스트.
   * @param err_func 연결 실패 시 호출할 에러 처리 함수(옵션).
   * @return 연결이 유효하면 true, 그렇지 않으면 false.
   */
  bool test_connection(std::function<void(sql::SQLException &e)> err_func = nullptr);

  // 연결 관리 쓰레드를 시작.
  bool start();

  // 연결 관리 쓰레드를 중지.
  bool stop ();

  /**
   * @brief 연결 해제된 커넥터를 등록.
   *
   * 등록된 커넥터는 재연결 대상이 됩니다.
   */
  void register_disconn()
  {
    disconns_.add(conn_tls_.get());
    waiter_.push_back(1); // 감시 쓰레드 즉시 깨우기
  }

//  int64_t connection_count() const
//  {
//    return connection_count_.load();
//  }

protected:
  /**
   * @brief 연결 테스트 함수.
   * @param conn_ptr 테스트할 연결 객체.
   * @return 연결이 유효하면 true, 그렇지 않으면 false.
   */
  static bool test(maria_conn_sptr conn_ptr);

  /**
   * @brief MariaDB 연결 생성.
   * @param url 접속 URL.
   * @param properties 접속 프로퍼티.
   * @return MariaDB 연결 객체(maria_conn_sptr).
   */
  static maria_conn_sptr
  connect(const sql::SQLString  &url,
          const sql::Properties &properties);

  /**
   * @brief 연결 관리 쓰레드 실행.
   */
  void run() override;

private:
//  std::atomic<int64_t> connection_count_{0};
  LockedObject<sql::SQLString>  url_;
  LockedObject<sql::Properties> properties_;

private:
  // AtomicSptr: 커넥션 포인터의 thread-safe한 교체
  // AtomicConnSptr: set 자료구조 사용을 위한 wrapper(AtomicSptr은 키를 가질 수 없는 구조.)
  using AtomicConn      = AtomicSptr<sql::Connection>;
  using AtomicConnSptr  = std::shared_ptr<AtomicConn>;

  // 핫패스: 정상적인 DB 쿼리 실행 경로 (pure lockfree)
  // 콜드패스(Cold path): DB 연결이 끊어져서 재연결하는 경로 (disconn add/del시 mutex)
  /**
   * @struct conn_tls_t
   * @brief TLS 기반 연결 관리 구조체.
   * @details 인스턴스별로 커넥터를 관리하도록 함.
   */
  TlInstance<AtomicConn> conn_tls_;

  /**
   * @struct disconns_t
   * @brief 연결 해제된 커넥터를 관리하는 구조체.
   */
  struct disconns_t
  {
    std::mutex lock; ///< 동기화를 위한 Mutex.
    std::set<AtomicConnSptr> conns; ///< 연결 해제된 커넥터 목록.

    /**
     * @brief 연결 해제된 커넥터 추가.
     * @param atomic_conn 추가할 커넥터.
     */
    void add(AtomicConnSptr atomic_conn)
    {
      std::lock_guard<std::mutex> guard(lock);
      conns.insert(atomic_conn);
    }

    /**
     * @brief 연결 해제된 커넥터 제거.
     * @param atomic_conn 제거할 커넥터.
     */
    void del(AtomicConnSptr atomic_conn)
    {
      std::lock_guard<std::mutex> guard(lock);
      conns.erase(atomic_conn);
    }

    /**
     * @brief 현재 연결 해제된 커넥터 목록 가져오기.
     * @return 연결 해제된 커넥터 목록.
     */
    std::set<AtomicConnSptr> load()
    {
      std::lock_guard<std::mutex> guard(lock);
      return conns;
    }
  };
  disconns_t disconns_; ///< 연결 해제된 커넥터 관리.
};


