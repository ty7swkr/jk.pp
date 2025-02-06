/*
 * MariaResultVal.h
 *
 *  Created on: 2024. 11. 23.
 *      Author: tys
 */

#pragma once

#include <extra/SysDateTime.h>
#include <mariadb/conncpp.hpp>
#include <memory>

/**
 * example
  try
  {
    MariaStatement selector(filter_db, "SELECT name, age FROM tb_user");
    MariaResultSet result = selector.execute_query();
    while (result.next())
    {
      std::string name;
      int age;

      // 연속 접근
      result >> name >> age;

      // 이름으로 접근
      name << result["name"];
      age  << result["age"];

      // 또는 인덱스로 접근
      name << result[1];
      age  << result[2];

      // as로 접근
      result[1].as_str();
    }
  }
  catch (sql::SQLException& e)
  {
    std::cout << "에러 발생: " << e.what() << std::endl;
    std::cout << "MySQL 에러 코드: " << e.getErrorCode() << std::endl;
    std::cout << "SQLState: " << e.getSQLState() << std::endl;
  }
*/

class MariaResultVal
{
public:
  MariaResultVal(size_t column_index, std::shared_ptr<sql::ResultSet> rs)
  : column_index(column_index), rs(rs) {}

  bool     is_null      ()  const { return rs->isNull     (column_index); }

  operator char         ()  const { return as_char  (); }
  operator int8_t       ()  const { return as_int8  (); }
  operator int16_t      ()  const { return as_int16 (); }
  operator int32_t      ()  const { return as_int32 (); }
  operator uint32_t     ()  const { return as_uint32(); }
  operator int64_t      ()  const { return as_int64 (); }
  operator uint64_t     ()  const { return as_uint64(); }
  operator bool         ()  const { return as_bool  (); }
  operator float        ()  const { return as_float (); }
  operator double       ()  const { return as_double(); }
  operator std::string  ()  const { return as_str   (); }
  operator SysDate      ()  const { return SysDate::from_string(as_str(), "%Y-%m-%d"); }
  operator SysDateTime  ()  const
  {
    std::string str = as_str();
    if (str.find('.') != std::string::npos)
      return SysDateTime::from_string_compact(str, "%Y-%m-%d %H:%M:%S.%L");
    return SysDateTime::from_string_compact(str, "%Y-%m-%d %H:%M:%S");
  }

  char        as_char   ()  const { auto str = as_str(); return  str.empty() ? '\0' : str[0]; };
  int8_t      as_int8   ()  const { return rs->getByte    (column_index); }
  int16_t     as_int16  ()  const { return rs->getShort   (column_index); }
  int32_t     as_int32  ()  const { return rs->getInt     (column_index); }
  uint32_t    as_uint32 ()  const { return rs->getUInt    (column_index); }
  int64_t     as_int64  ()  const { return rs->getInt64   (column_index); }
  // mariadb getUInt64는 메모리 오류 버그가 있습니다.
//  uint64_t    as_uint64 ()  const { return rs->getUInt64  (column_index); }
  uint64_t    as_uint64 ()  const { return std::atoll(rs->getString(column_index).c_str()); }
  int8_t      as_bool   ()  const { return rs->getBoolean (column_index); }
  float       as_float  ()  const { return rs->getFloat   (column_index); }
  double      as_double ()  const { return rs->getDouble  (column_index); }
  std::string as_string ()  const { return as_str();   }

  // 축약형
  int8_t      as_byte   ()  const { return as_int8();  }
  int32_t     as_int    ()  const { return as_int32(); }
  std::string as_str    ()  const { return std::string(rs->getString(column_index)); }

public:
  size_t   column_index = 0;
  std::shared_ptr<sql::ResultSet> rs;
};

