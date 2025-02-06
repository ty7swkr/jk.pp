/*
 * MariaStatement.h
 *
 *  Created on: 2024. 11. 20.
 *      Author: tys
 */

#pragma once

#include <extra/MariaConnectorTls.h>
#include <mariadb/conncpp.hpp>
#include <mysql/mysqld_error.h>
#include <mysql/errmsg.h>
#include <string>
#include <memory>

// PreparedStatement 래퍼 클래스
/**
 * @brief sql::ResultSet 랩핑클래스
 * @details sql::ResultSet를 랩핑하여 템플릿 적용하여 타입을 컴파일타임에 적용하도록 작성.
 *
    std::string a = "1234";
    char b[100] = "hello";
    int c = 4;

    try
    {
      MariaStatement stmt(db_conns, "SELECT a, b, c FROM t where f = ?");
      stmt << 3;

      MariaResultSet rs = stmt.execute_query();
      while (rs.next() == true)
      {
        rs >> a >> b >> c;
        or
        a = rs[1];
        b = rs["b"];
        c = rs[3];
      }
    catch (sql::SQLException &e) //sql::SQLSyntaxErrorException
    {
      std::cout << e.what() << std::end;
      std::cout << e.getErrorCode() << std::endl;
      std::cout << e.getSQLState() << std::endl;
    }
 *
 */
class MariaStatement
{
public:
  MariaStatement(MariaConnectorTls &tls, const std::string &query)
  : tls_(tls), index_(1)
  {
    try
    {
      conn_ = tls_.get_connector();
      pstmt_.reset(conn_->prepareStatement(query));
    }
    catch (const sql::SQLException &e) //sql::SQLSyntaxErrorException
    {
      exception_proc(e);
      throw;
    }
  }

  // 인덱스로 컬럼 지정 1부터 시작인거임.
  MariaStatement &operator[](int index)
  {
    index_ = index;
    return *this;
  }

  // 다양한 타입 처리를 위한 연산자 오버로딩
  MariaStatement &operator<<(std::nullptr_t           ) { pstmt_->setNull   (index_++, 0);     return *this; } // NULL 값 처리
  MariaStatement &operator<<(const bool         &value) { pstmt_->setBoolean(index_++, value); return *this; }
  MariaStatement &operator<<(const int8_t       &value) { pstmt_->setByte   (index_++, value); return *this; }
  MariaStatement &operator<<(const int16_t      &value) { pstmt_->setShort  (index_++, value); return *this; }
  MariaStatement &operator<<(const int32_t      &value) { pstmt_->setInt    (index_++, value); return *this; }
  MariaStatement &operator<<(const uint32_t     &value) { pstmt_->setLong   (index_++, value); return *this; }
  MariaStatement &operator<<(const int64_t      &value) { pstmt_->setInt64  (index_++, value); return *this; }
  MariaStatement &operator<<(const uint64_t     &value) { pstmt_->setUInt64 (index_++, value); return *this; }
  MariaStatement &operator<<(const float        &value) { pstmt_->setFloat  (index_++, value); return *this; }
  MariaStatement &operator<<(const double       &value) { pstmt_->setDouble (index_++, value); return *this; }
  MariaStatement &operator<<(const std::string  &value) { pstmt_->setString (index_++, value); return *this; }
  MariaStatement &operator<<(const char         *value) { pstmt_->setString (index_++, value); return *this; }
  template<size_t N>
  MariaStatement &operator<<(const char    (&value)[N]) { pstmt_->setString (index_++, std::string(value, N-1)); return *this; }

  // 실행 메서드들
  void    execute();

  // for INSERT, UPDATE, DELETE
  int32_t execute_update();

  // for select
  std::shared_ptr<sql::ResultSet>
          execute_query(const size_t &fetch_size = 1000);

  // 파라미터 인덱스 리셋 (재사용시 필요)
  void    reset();

  sql::PreparedStatement &stmt() { return *(pstmt_.get()); }

protected:
  void exception_proc(const sql::SQLException &e)
  {
    // 서버 오류시 재접속 후 재시도 - legacy code
    //    MYSQL_ERR_SUCCESS = 0,
    //    MYSQL_ERR_WSREP_NOT_YET_PREPARED = 1047,
    //    MYSQL_ERR_ILLEGAL_MIX_OF_COLLATIONS = 1271, // DB 접속이 불안하면, 이 에러가 발생하는 경우가 있음.
    //    MYSQL_ERR_CONNECTION_KILLED = 1927, // Connection killed by MaxScale: Router could not recover from connection errors
    //    MYSQL_ERR_CONNECTION_REFUSED = 2003,
    //    MYSQL_ERR_SEVER_GONE_AWAY = 2006,
    //    MYSQL_ERR_LOST_CONNECTION = 2013,
    //    MYSQL_ERR_ALREADY_CONNECTED = 2058,

    //  src/ExceptionMapper.cpp
    //    01002: 연결 끊기 오류(SQLState값들 정리)
    //    클래스 08 - 연결 예외:
    //
    //    08001: 서버 연결 실패 (SQL client unable to establish SQL connection)
    //    08003: 연결이 더 이상 존재하지 않음 (Connection does not exist)
    //    08004: 서버가 연결 거부 (Server rejected the connection)
    //    08006: 연결 실패 (Connection failure)
    //    08007: 트랜잭션 resolution unknown 상태에서 연결 종료
    //    08S01: 통신 링크 실패 (Communication link failure)
    //
    //    클래스 01 중 연결 관련:
    //    01002: 연결 해제 에러 (Disconnect error)
    //
    //    클래스 57 - 운영자 개입:
    //    57P01: 관리자에 의한 강제 중단 (admin shutdown)
    //    57P02: 충돌 종료 (crash shutdown)
    //    57P03: 서버를 사용할 수 없음 (cannot connect now)
    //
    static const auto conn_err_maria = [](const sql::SQLException &e) -> bool
    {
      switch (const_cast<sql::SQLException&>(e).getErrorCode())
      {
        case ER_UNKNOWN_COM_ERROR:
        case ER_CANT_AGGREGATE_NCOLLATIONS:
        case ER_CONNECTION_KILLED:
        case CR_CONNECTION_ERROR:
        case CR_CONN_HOST_ERROR:
        case CR_UNKNOWN_HOST:
        case CR_SERVER_GONE_ERROR:
        case CR_SERVER_LOST:
        case CR_ALREADY_CONNECTED: // CR_ALREADY_CONNECTED 왜? 그래서 일단 끊기라고...
          return true;
        default:
          return true;
      }
    };

    // 위에서 안걸려? 그럼 여기서라도 걸려라!
    static const auto conn_err_sqlstate = [](const sql::SQLException &e) -> bool
    {
      std::string sql_state = const_cast<sql::SQLException&>(e).getSQLStateCStr();
      if (sql_state == "01002" || sql_state == "08001" || sql_state == "08003" || sql_state == "08004" ||
          sql_state == "08006" || sql_state == "08007" || sql_state == "08S01" ||
          sql_state == "57P01" || sql_state == "57P02" || sql_state == "57P03")
        return true;
      return false;
    };

    if (conn_err_maria(e) == true || conn_err_sqlstate(e) == true)
      tls_.register_disconn();
  }

private:
  maria_conn_sptr   conn_;
  MariaConnectorTls &tls_;

  // 다른곳에서 해제되더라도 살아있게 하기 위해
  std::unique_ptr<sql::PreparedStatement> pstmt_ = nullptr;
  int index_ = 1;
};

// 실행 메서드들
inline void
MariaStatement::execute()
{
  try
  {
    pstmt_->execute();
  }
  catch (const sql::SQLException &e)
  {
    exception_proc(e);
    throw;
  }
}

// for INSERT, UPDATE, DELETE
inline int32_t
MariaStatement::execute_update()
{
  try
  {
    return pstmt_->executeUpdate();
  }
  catch (const sql::SQLException &e)
  {
    exception_proc(e);
    throw;
  }

  return 0;
}

// for select
inline std::shared_ptr<sql::ResultSet>
MariaStatement::execute_query(const size_t &fetch_size)
{
  try
  {
    pstmt_->setFetchSize(fetch_size);
    return std::shared_ptr<sql::ResultSet>(pstmt_->executeQuery());
  }
  catch (const sql::SQLException &e)
  {
    exception_proc(e);
    throw;
  }

  return nullptr;
}

// 파라미터 인덱스 리셋 (재사용시 필요)
inline void
MariaStatement::reset()
{
  index_ = 1;
}


