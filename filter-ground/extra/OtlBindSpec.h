/*
 * OtlBindSpec.h
 *
 *  Created on: 2024. 12. 11.
 *      Author: tys
 */

#pragma once

#include <extra/OtlConnectorTls.h>
#include <extra/helper.h>
#include <algorithm>
#include <map>
#include <memory>

// example
// otl_connect db;
// db.rlogon("oraspam/pltspam10!@//210.217.178.121:1521/spamdb"));
//
// // 방법 1: 직접 연결 객체 전달
// OtlBindSpec emp("EMPLOYEES", db);
//
// // 방법 2: shared_ptr로 연결 객체 전달
// auto db_ptr = std::make_shared<otl_connect>(db);
// OtlBindSpec dept("DEPARTMENTS", db_ptr);
//
// // 사용 예제:
// std::string query =
//     "UPDATE  employees SET "
//     "        first_name    = " + emp["first_name"] + ", "// :first_name_1<char[20]>
//     "        salary        = " + emp["salary"] + ", "    // :salary_2<double>
//     "WHERE   first_name    = " + emp["first_name"];      // :first_name_3<char[20]>
//
// otl_stream selector(1000, query, conn);
// selector << "second" << "salary" << "first";
//
// 메타데이터 정보 출력
// std::cout << "Employees table metadata:\n" << emp.to_string() << std::endl;
//===============================================================================
// example2
//  class OracleDB : public OtlConnectorTls, public Singleton<OracleDB> {};
//
//#define oradb OracleDB::ref()
//
//  oradb.set_connection_info("oraspam/pltspam10!@//1.1.8.1:1521/spamdb");
//  {
//    d_log << oradb.start();
//    try
//    {
//      auto conn = oradb.get_connector();
//      static OtlBindSpec table("PLAN_TABLE", "spamdb", conn);
//      d_log << "\n" + table.to_string();
//      d_log << table.length("FILTER_PREDICATES");
//    }
//    catch (const otl_exception &e)
//    {
//      d_log << "code: " << e.code;      // 오라클 에러 코드
//      d_log << "msg : " << e.msg;       // 오라클 에러 메시지
//      d_log << "sql : " << e.stm_text;  // 실행하려던 SQL문
//      d_log << "var : " << e.var_info;  // 바인드 변수 정보
//    }
//  }
//
//  oradb.stop();
//  return 0;
//  return 0;
//

/**
 * @brief OTL 오라클 바인딩에 필요한 스트링 생성
 *
 * 테이블의 컬럼 정보를 조회하고, OTL 바인딩에 필요한 타입 정보를 제공합니다.
 */
class OtlBindSpec
{
public:
  /**
   * @brief 현재 접속된 데이터베이스의 테이블 메타데이터를 조회합니다.
   * @param name 테이블명
   * @param conn OTL 데이터베이스 연결 객체
   */
  OtlBindSpec(const std::string& name, otl_connect &conn)
  : table_name_(name) { load(conn); }

  /**
   * @brief shared_ptr로 전달된 연결을 사용하여 테이블 메타데이터를 조회합니다.
   * @param name 테이블명
   * @param conn_sptr OTL 데이터베이스 연결 객체의 shared_ptr
   */
  OtlBindSpec(const std::string& name, std::shared_ptr<otl_connect> conn_sptr)
  : table_name_(name) { load(*conn_sptr.get()); }

  /**
   * @brief 지정된 데이터베이스의 테이블 메타데이터를 조회합니다.
   * @param name 테이블명
   * @param database 데이터베이스 링크명
   * @param conn OTL 데이터베이스 연결 객체
   */
  OtlBindSpec(const std::string& name, const std::string &database, otl_connect &conn)
  : table_name_(name), database_(database) { load(conn); }

  /**
   * @brief shared_ptr로 전달된 연결을 사용하여 지정된 데이터베이스의 메타데이터를 조회합니다.
   * @param name 테이블명
   * @param database 데이터베이스 링크명
   * @param conn_sptr OTL 데이터베이스 연결 객체의 shared_ptr
   */
  OtlBindSpec(const std::string& name, const std::string &database, std::shared_ptr<otl_connect> conn_sptr)
  : table_name_(name), database_(database) { load(*conn_sptr.get()); }

  virtual ~OtlBindSpec() {}

  void reload(otl_connect &conn)
  {
    load(conn);
  }

  void reload(std::shared_ptr<otl_connect> conn_sptr)
  {
    load(*conn_sptr.get());
  }

  /**
   * @brief 컬럼에 대한 OTL 바인딩 문자열을 생성합니다.
   * @param col_name 컬럼명
   * @return ":컬럼명_인덱스<타입>" 형식의 바인딩 문자열
   * @throws std::out_of_range 존재하지 않는 컬럼명이 전달된 경우
   */
  std::string operator[](const std::string &col_name) const
  {
    if (column_types_.count(col_name) == 0)
      return ":__ERROR__" + col_name + "<NOT_FOUND>";

    std::string result = ":" + col_name + "_" + std::to_string(++index_);

    return result + column_types_.at(col_name);
  }

  /**
   * @brief 문자열 타입 컬럼의 길이를 반환합니다.
   * @details CHAR, VARCHAR2, NCHAR, NVARCHAR2 타입에 대해서만 유효한 길이를 반환합니다.
   * @param col_name 컬럼명
   * @return 문자열 길이 (바이트 단위), 컬럼이 존재하지 않거나 문자열 타입이 아닌 경우 0 반환
   */
  size_t length(const std::string &col_name) const
  {
    if (column_sizes_.count(col_name) == 0)
      return 0;
    return column_sizes_.at(col_name);
  }

  /**
   * @brief 모든 컬럼의 바인딩 정보를 문자열로 반환합니다.
   * @return 각 컬럼의 바인딩 문자열을 개행으로 구분한 전체 문자열
   */
  std::string to_string() const
  {
    std::string result;
    for (const auto &pair : column_types_)
      result += ":" + pair.first + column_types_.at(pair.first) + "\n";

    return result;
  }

protected:
  //  - Oracle에는 LONG 타입이 있지만 이는 레거시(legacy) 타입입니다.
  //  - Oracle LONG 타입은:
  //  최대 2GB까지 저장 가능한 가변길이 문자열 타입
  //  테이블당 하나의 LONG 컬럼만 가능
  //  여러 제약사항이 있어서 사용이 권장되지 않음
  //
  //  - Oracle에서는 LONG 대신:
  //  LONG -> CLOB
  //  LONG RAW -> BLOB
  //
  //  - native 바인딩 타입
  //  NUMBER(p, 0) 일 때
  //
  //  p <= 9: <int> -> C++의 int (32bit integer)
  //  p > 9: <bigint> -> C++의 int64_t
  //
  //  NUMBER(p,s) where s > 0
  //
  //  <double> -> C++의 double
  //
  //  - 문자열 타입들:
  //  VARCHAR2, CHAR -> <char[N]>
  //  LONG -> <clob> -> OTL의 otl_lob_stream
  //  CLOB -> <clob> -> OTL의 otl_lob_stream

  /**
   * @brief Oracle 데이터 타입을 OTL 바인딩 타입으로 변환합니다.
   *
   * Oracle 타입과 C++ 타입의 매핑:
   * - NUMBER(p,0): p <= 9이면 int, p > 9이면 int64_t
   * - NUMBER(p,s): s > 0이면 double
   * - VARCHAR2, CHAR: char[N]
   * - NVARCHAR2, NCHAR: char[N] (유니코드 지원)
   * - DATE, TIMESTAMP: timestamp
   * - CLOB, LONG: otl_lob_stream
   * - RAW: raw[N]
   * - LONG RAW: blob
   *
   * @param col_name 컬럼명
   * @param data_type Oracle 데이터 타입
   * @param data_length 데이터 길이
   * @param data_precision 숫자형 데이터의 전체 자릿수
   * @param data_scale 숫자형 데이터의 소수점 이하 자릿수
   * @return OTL 바인딩 타입 문자열 (예: "<char[50]>", "<int>", "<double>")
   *
   * @note LONG 타입은 레거시이며 CLOB 사용을 권장
   */
  virtual std::string
  to_otl_type(const std::string &col_name,
              const std::string &data_type,
              const int32_t     &data_length,
              const int32_t     &data_precision,
              const int32_t     &data_scale)
  {
#ifdef OTL_ADD_NULL_TERMINATOR_TO_STRING_SIZE
    auto length = data_length;
#else
    auto length = data_length+1;
#endif
    switch (string_hash(data_type))
    {
      // 오라클은 unsigned를 지원하지 않는다고.....
      case constexpr_hash("NUMBER") :
      {
        if (data_scale > 0)
          return "<double>";
        if (data_precision >= 0 && data_precision  <= 4)
          return "<short int>";
        if (data_precision >  5 && data_precision  <= 9)
          return "<int>";
        return "<long int>";
      }
      case constexpr_hash("VARCHAR")    :
      case constexpr_hash("VARCHAR2")   :
      case constexpr_hash("CHAR")       : column_sizes_[col_name] = length; return "<char[" + std::to_string(length) + "]>";
      case constexpr_hash("NVARCHAR2")  :
      case constexpr_hash("NCHAR")      : column_sizes_[col_name] = length; return "<char[" + std::to_string(length) + "]>";
      case constexpr_hash("DATE")       :
      case constexpr_hash("TIMESTAMP")  : return "<timestamp>";
      case constexpr_hash("RAW")        : return "<raw[" + std::to_string(length) + "]>";
      case constexpr_hash("CLOB")       :
      case constexpr_hash("LONG")       : return "<clob>";
      case constexpr_hash("LONG RAW")   : return "<blob>";
      default: return "<"+col_name+":NONE-ERROR>";
    }

    return "<"+col_name+":NONE-ERROR>";  // 기본값???
  }

  /**
   * @brief 테이블의 메타데이터를 데이터베이스에서 조회합니다.
   * @param conn OTL 데이터베이스 연결 객체
   * @throws otl_exception 데이터베이스 조회 실패 시
   */
  virtual void
  load(otl_connect &conn)
  {
    try
    {
      otl_stream selector(1000,  // 버퍼 사이즈
                          ("SELECT   column_name, "
                           "         data_type, "
                           "         data_length, "
                           "         data_precision, "
                           "         data_scale "
                           "FROM     all_tab_columns" + (database_.empty() ? " " : "@" + database_ + " ") +
                           "WHERE    table_name = :table_name<char[50]> "
                           "ORDER BY column_id").c_str(),
                          conn);

      selector << table_name_;

      std::string col_name;
      std::string data_type;
      int32_t     data_length;
      int32_t     data_precision;
      int32_t     data_scale;

      while (!selector.eof())
      {
        // 컬럼 정보 읽기
        selector >> col_name >> data_type >> data_length >> data_precision >> data_scale;

        // Oracle 타입을 OTL 바인딩 타입으로 매핑
        std::string otl_type = to_otl_type(col_name, data_type, data_length, data_precision, data_scale);

        // 매핑 정보 저장
        column_types_[col_name] = otl_type;
      }
    }
    catch (const otl_exception& e)
    {
      throw;
    }
  }

protected:
  struct case_insensitive_compare
  {
    bool operator()(const std::string& a, const std::string& b) const
    {
      return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                          [](char c1, char c2) { return std::tolower(c1) < std::tolower(c2); } );
    }
  };
  mutable uint16_t index_ = 0;                       ///< 바인딩 변수의 고유 인덱스
  std::string table_name_;                           ///< 테이블명
  std::string database_;                             ///< 데이터베이스 링크명
  std::map<std::string, std::string, case_insensitive_compare> column_types_;  ///< 컬럼명 -> OTL 바인딩 타입 매핑
  std::map<std::string, size_t, case_insensitive_compare>      column_sizes_;  ///< 컬럼명 -> 데이터 길이 매핑
};




