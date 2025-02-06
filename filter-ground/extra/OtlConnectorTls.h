/*
 * OtlConnectorTls.h
 *
 *  Created on: 2024. 11. 20.
 *      Author: tys
 */

#pragma once

#define OTL_STL
#define OTL_CPP_11_ON

#define OTL_ADD_NULL_TERMINATOR_TO_STRING_SIZE

#define OTL_ORA11G_R2

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#include <otlv4.h>
#pragma GCC diagnostic pop

#include <extra/BlockingDequeThread.h>
#include <extra/SpinLockGuard.h>
#include <extra/LockedObject.h>
#include <extra/AtomicSptr.h>
#include <extra/Singleton.h>
#include <extra/TlInstance.h>

#include <memory>
#include <set>

/** example
class OracleDB : public OtlConnectorTls, public Singleton<OracleDB> {};

#define oradb OracleDB::ref()

int main(int argc, char** argv)
{
  oradb.set_connection_info("username/password@//1.1.8.1:1521/spamdb");
  {
    d_log << oradb.start();
    try
    {
      auto conn = oradb.get_connector();
    }
    catch (const otl_exception &e)
    {
      d_log << "code: " << e.code;      // 오라클 에러 코드
      d_log << "msg : " << e.msg;       // 오라클 에러 메시지
      d_log << "sql : " << e.stm_text;  // 실행하려던 SQL문
      d_log << "var : " << e.var_info;  // 바인드 변수 정보
    }
  }
  oradb.stop();
  return 0;
 */

using otl_conn_sptr = std::shared_ptr<otl_connect>;

class OtlConnectorTls : protected BlockingDequeThread<>
{
public:
  OtlConnectorTls();
  virtual ~OtlConnectorTls();

  // 접속 실패 시 호출되는 알람 콜백 함수.
  std::function<void(const otl_exception &e)> occur_connect_error = nullptr;

  // 모든 커넥터가 성공적으로 연결된 후 호출되는 콜백 함수.
  std::function<void()> clear_connect_error = nullptr;

  // connection info를 재접속시 변경하고 싶을때 사용하는 함수.
  // 재접속시 이 람다함수가 설정되어 있으면 호출하여 준다.
  std::function<void()> reset_connection_info = nullptr;

  /**
   * @brief Oracle 접속 정보를 설정.
   * @param 접속 스트링. username/password@//localhost:1521/XE
   */
  void set_connection_info(const std::string &rlogon_str)
  {
    rlogon_str_ = rlogon_str;
  }

  std::string rlogon_str() const { return rlogon_str_.load(); }

  /**
   * @brief Oracle Connector 인스턴스를 반환.
   *
   * @details 필요 시 Test 쿼리를 실행하고 실패하면 재연결을 시도합니다.
   * 재연결 실패 시 연결 쓰레드에서 처리를 요청하고 예외를 던집니다.
   *
   * @return Oracle 연결 객체(otl_conn_sptr).
   * @throw const otl_exception 연결 실패 시 예외 발생.
   */
  otl_conn_sptr get_connector();

  /**
   * @brief 연결 상태 테스트.
   * @param err_func 연결 실패 시 호출할 에러 처리 함수(옵션).
   * @return 연결이 유효하면 true, 그렇지 않으면 false.
   */
  bool test_connection(std::function<void(const otl_exception &e)> err_func = nullptr);

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
  static bool test   (otl_conn_sptr conn_ptr);

  /**
   * @brief Oracle 연결 생성.
   * @param 접속 스트링.
   * @return Oracle 연결 객체(otl_conn_sptr).
   */
  static otl_conn_sptr
  connect(const std::string &rlogon_str);

  /**
   * @brief 연결 관리 쓰레드 실행.
   */
  void run() override;

private:
//  std::atomic<int64_t> connection_count_{0};
  LockedObject<std::string>  rlogon_str_;

private:
  // AtomicSptr: 커넥션 포인터의 thread-safe한 교체
  // AtomicConnSptr: set 자료구조 사용을 위한 wrapper(AtomicSptr은 키를 가질 수 없는 구조.)
  using AtomicConn      = AtomicSptr<otl_connect>;
  using AtomicConnSptr  = std::shared_ptr<AtomicConn>;

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


