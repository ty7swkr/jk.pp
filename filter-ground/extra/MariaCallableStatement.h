#pragma once

#include <extra/MariaConnectorTls.h>
#include <string>
#include <memory>

/**
 * @brief SQL 타입에 맞는 sql::Types를 얻기 위한 traits 구조체.
 * @details MariaDB Connector API의 프로시저 사용 시 OUT 파라미터 타입 매핑을 제공합니다.
 * mariadb connector api에서 프러시져 사용시 out 파라미터의 타입이 이것 뿐입니다.
 */
template<typename T>  struct sql_type_of;
template<>            struct sql_type_of<bool>          { static const sql::Types value = sql::BOOLEAN; };
template<>            struct sql_type_of<int8_t>        { static const sql::Types value = sql::CHAR;    };
template<>            struct sql_type_of<int16_t>       { static const sql::Types value = sql::SMALLINT;};
template<>            struct sql_type_of<int32_t>       { static const sql::Types value = sql::INTEGER; };
template<>            struct sql_type_of<int64_t>       { static const sql::Types value = sql::BIGINT;  };
template<>            struct sql_type_of<float>         { static const sql::Types value = sql::DECIMAL; };
template<>            struct sql_type_of<double>        { static const sql::Types value = sql::DECIMAL; };
template<>            struct sql_type_of<std::string>   { static const sql::Types value = sql::VARCHAR; };
template<>            struct sql_type_of<const char *>  { static const sql::Types value = sql::VARCHAR; };
template<>            struct sql_type_of<std::nullptr_t>{ static const sql::Types value = sql::SQLNULL; };

/**
 * @class MariaOutParam
 * @brief MariaDB OUT 파라미터를 관리하는 클래스.
 *
 * @tparam T OUT 파라미터 타입.
 * @details mariadb connector cpp 라이브러리 버그로
 * select + out 조합의 결과는 비정상동작.
 * 프로시저 + in/out 정상동작
 * 프로시저 + in/select 정상동작
 * 프로시저 + out/select - 비정상동작
 */
template<typename T>
class MariaOutParam
{
public:
  /**
   * @brief 생성자.
   * @param index OUT 파라미터의 인덱스.
   */
  explicit MariaOutParam(int32_t index) : index(index) {}

  int32_t index; // OUT 파라미터 인덱스.
  using value_type = T; // 파라미터 타입 정의. 컴파일타임에 사용됨.

  friend class MariaCallableStatement;
};

/**
 * @class MariaCallableStatement
 * @brief MariaDB 프로시저 호출을 위한 CallableStatement 래퍼.
 */
class MariaCallableStatement
{
public:
  /**
   * @brief 생성자.
   * @param tls MariaDB Connector TLS 객체.
   * @param procedure_query 호출할 프로시저 쿼리 문자열.
   */
  MariaCallableStatement(MariaConnectorTls &tls, const std::string& procedure_query)
  : tls_(tls), index_(1)
  {
    try
    {
      conn_ = tls_.get_connector();
      cstmt_.reset(conn_->prepareCall(procedure_query));
    }
    catch (const sql::SQLException &e)
    {
      exception_proc(e);
      throw;
    }
  }

  /**
   * @brief 인덱스를 사용하여 파라미터 지정.
   * @param index 파라미터 인덱스.
   * @return 현재 MariaCallableStatement 객체 참조.
   */
  MariaCallableStatement &operator[](int index)
  {
    index_ = index;
    return *this;
  }

  /**
   * @brief IN 파라미터를 바인딩.
   * @details 다양한 데이터 타입에 대한 오버로딩 지원.
   * @param value 입력값.
   * @return 현재 MariaCallableStatement 객체 참조.
   */
  MariaCallableStatement &in_param(std::nullptr_t           ) { cstmt_->setNull   (index_++, 0);     return *this; }
  MariaCallableStatement &in_param(const bool         &value) { cstmt_->setBoolean(index_++, value); return *this; }
  MariaCallableStatement &in_param(const int8_t       &value) { cstmt_->setByte   (index_++, value); return *this; }
  MariaCallableStatement &in_param(const int16_t      &value) { cstmt_->setShort  (index_++, value); return *this; }
  MariaCallableStatement &in_param(const int32_t      &value) { cstmt_->setInt    (index_++, value); return *this; }
  MariaCallableStatement &in_param(const uint32_t     &value) { cstmt_->setUInt   (index_++, value); return *this; }
  MariaCallableStatement &in_param(const int64_t      &value) { cstmt_->setInt64  (index_++, value); return *this; }
  MariaCallableStatement &in_param(const uint64_t     &value) { cstmt_->setUInt64 (index_++, value); return *this; }
  MariaCallableStatement &in_param(const float        &value) { cstmt_->setFloat  (index_++, value); return *this; }
  MariaCallableStatement &in_param(const double       &value) { cstmt_->setDouble (index_++, value); return *this; }
  MariaCallableStatement &in_param(const std::string  &value) { cstmt_->setString (index_++, value); return *this; }
  MariaCallableStatement &in_param(const char         *value) { cstmt_->setString (index_++, value); return *this; }
  template<size_t N>
  MariaCallableStatement &in_param(const char (&value)[N])
  {
    cstmt_->setString(index_++, std::string(value, N-1));
    return *this;
  }

  /**
   * @brief OUT 파라미터를 등록하고 반환.
   * @details OUT 파라미터 등록 - OutParam  반환, 아래 특수화 참조할것!!
   * @tparam T 파라미터 타입.
   * @return MariaOutParam<T> 객체.
   */
  template<typename T> MariaOutParam<T>
  out_param()
  {
    cstmt_->registerOutParameter(index_, sql_type_of<T>::value);
    return MariaOutParam<T>(index_++);
  }

  // OutParam로 결과값 얻기
  // OUT 파라미터 값 가져오기 - 타입별 오버로딩
  /**
   * @brief OUT 파라미터 값을 가져옴.
   *
   * @tparam T 파라미터 타입.
   * @param result MariaOutParam 객체.
   * @return OUT 파라미터 값.
   */
  bool            get_out_param(const MariaOutParam<bool>           &result) const { return cstmt_->getBoolean (result.index); }
  int8_t          get_out_param(const MariaOutParam<int8_t>         &result) const { return cstmt_->getByte    (result.index); }
  int16_t         get_out_param(const MariaOutParam<int16_t>        &result) const { return cstmt_->getShort   (result.index); }
  int32_t         get_out_param(const MariaOutParam<int32_t>        &result) const { return cstmt_->getInt     (result.index); }
  int64_t         get_out_param(const MariaOutParam<int64_t>        &result) const { return cstmt_->getLong    (result.index); }
  float           get_out_param(const MariaOutParam<float>          &result) const { return cstmt_->getFloat   (result.index); }
  double          get_out_param(const MariaOutParam<double>         &result) const { return cstmt_->getDouble  (result.index); }
  std::string     get_out_param(const MariaOutParam<std::string>    &result) const { return cstmt_->getString  (result.index).c_str(); }
  std::nullptr_t  get_out_param(const MariaOutParam<std::nullptr_t> &result) const { return nullptr; }

  /**
   * @brief 프로시저를 실행하고 Update 결과를 반환.
   * @return 업데이트된 행의 수.
   * @throw sql::SQLException 실행 실패 시 예외 발생.
   */
  int32_t
  execute_update()
  {
    try
    {
      int32_t res = cstmt_->executeUpdate();
      return res;
    }
    catch (const sql::SQLException &e)
    {
      exception_proc(e);
      throw;
    }
    return 0;
  }

  /**
   * @brief 프로시저 실행 결과를 반환. OUT Parameter call
   * @return SQL ResultSet 객체의 공유 포인터.
   * @throw sql::SQLException 실행 실패 시 예외 발생.
   */
  std::shared_ptr<sql::ResultSet>
  execute_query()
  {
    try
    {
      return std::shared_ptr<sql::ResultSet>(cstmt_->executeQuery());
    }
    catch (const sql::SQLException &e)
    {
      exception_proc(e);
      throw;

    }
    return nullptr;
  }

  void reset() { index_ = 1; }

  const int &index() const { return index_; }

protected:
  /**
   * @brief SQLException 처리 함수.
   * @param e 발생한 SQLException 객체.
   */
  void exception_proc(const sql::SQLException &e)
  {
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
  int               index_ = 1;

private:
  std::shared_ptr<sql::CallableStatement> cstmt_;
};

template<> inline MariaOutParam<std::string>
MariaCallableStatement::out_param()
{
//  in_param(std::string()); --index_; // 더미값을 세팅하지 않으면 예외를 뱉는다.
  cstmt_->registerOutParameter(index_, sql_type_of<std::string>::value);
  return MariaOutParam<std::string>(index_++);
}

template<> inline MariaOutParam<const char *>
MariaCallableStatement::out_param()
{
//  in_param(""); --index_; // 더미값을 세팅하지 않으면 예외를 뱉는다.
  cstmt_->registerOutParameter(index_, sql_type_of<const char *>::value);
  return MariaOutParam<const char *>(index_++);
}









