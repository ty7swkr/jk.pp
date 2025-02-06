/*
 * MariaResultSet.h
 *
 *  Created on: 2024. 11. 20.
 *      Author: tys
 */

#pragma once

#include <extra/MariaConnectorTls.h>
#include <extra/MariaResultVal.h>
#include <cstring>

/**
 * @brief sql::ResultSet 랩핑클래스, SQL 결과 셋을 처리하고 데이터를 추출하기 위한 래퍼 클래스.
 * @details sql::ResultSet를 랩핑하여 템플릿 적용하여 타입을 컴파일 타임에 적용하도록 작성.
 * 이 클래스는 SQL 결과 셋과 상호작용하는 인터페이스를 제공합니다.
 * 컬럼 인덱스나 이름으로 데이터를 추출하고, NULL 값을 처리하며,
 * 결과 셋의 행(row)을 순회하는 기능을 지원합니다.
 *
    try
    {
      MariaStatement stmt(db_conns, "SELECT a, b, c FROM t where f = ?");
      stmt << 3;

      MariaResultSet rs = stmt.execute_query();
      while (rs.next() == true)
      {
        rs >> a >> b >> c;
      }
      또는
      while (rs.next() == true)
      {
        a = rs["a"];
        b = rs["b"];
        c = rs["c"];
      }
      또는
      while (rs.next() == true)
      {
        a = rs[1];
        b = rs[2];
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
class MariaResultSet
{
public:
  /**
   * @brief MariaResultSet 생성자.
   * @param rs sql::ResultSet의 shared_ptr 객체.
   */
  MariaResultSet(std::shared_ptr<sql::ResultSet> rs) : rs_(std::move(rs)) {}

  /**
   * @brief 결과 셋이 NULL이거나 현재 값이 NULL인지 확인.
   * @return 결과 셋 또는 현재 값이 NULL이면 true, 그렇지 않으면 false.
   */
  bool is_null() const
  {
    if (rs_ == nullptr) return true;
    return rs_->isNull(column_);
  }

  /**
   * @brief 컬럼 값을 변수에 추출.
   *
   * 데이터베이스에서 해당 컬럼 값이 NULL인 경우:
   * - 정수형 타입은 0으로 설정됩니다.
   * - 문자열 타입은 빈 문자열("")로 설정됩니다.
   *
   * @param value 값을 저장할 변수.
   * @return 현재 MariaResultSet 객체에 대한 참조.
   * @throws sql::SQLException 값 추출 중 오류가 발생하면 예외를 던짐.
   */
  const MariaResultSet &operator>>(int8_t       &value) const { value = rs_->getByte   (column_++); return *this; }
  const MariaResultSet &operator>>(int16_t      &value) const { value = rs_->getShort  (column_++); return *this; }
  const MariaResultSet &operator>>(int32_t      &value) const { value = rs_->getInt    (column_++); return *this; }
  const MariaResultSet &operator>>(uint32_t     &value) const { value = rs_->getUInt   (column_++); return *this; }
  const MariaResultSet &operator>>(long long    &value) const { value = rs_->getInt64  (column_++); return *this; }
  const MariaResultSet &operator>>(int64_t      &value) const { value = rs_->getInt64  (column_++); return *this; }
  const MariaResultSet &operator>>(uint64_t     &value) const { value = rs_->getUInt64 (column_++); return *this; }
  const MariaResultSet &operator>>(float        &value) const { value = rs_->getFloat  (column_++); return *this; }
  const MariaResultSet &operator>>(double       &value) const { value = rs_->getDouble (column_++); return *this; }
  const MariaResultSet &operator>>(std::string  &value) const { value = rs_->getString (column_++); return *this; }

  /**
   * @brief 고정 길이 char 배열에 문자열 값을 복사.
   *
   * @tparam N 배열 크기.
   * @param value 값을 저장할 char 배열.
   * @return 현재 MariaResultSet 객체에 대한 참조.
   */
  template<size_t N>
  const MariaResultSet &operator>>(char (&value)[N]) const
  {
    memset(value, 0x00, N);
    std::string str(rs_->getString(column_++));
    std::strncpy(value, str.c_str(), std::min(N - 1, str.length()));
    return *this;
  }

  /**
   * @brief 1부터 시작하는 인덱스로 컬럼 지정.
   * @param index 1부터 시작하는 컬럼 인덱스.
   * @return 지정된 컬럼 값을 나타내는 MariaResultVal 객체.
   */
  MariaResultVal operator[](int32_t index)
  {
    column_ = index + 1;
    return MariaResultVal(index, rs_);
  }

  /**
   * @brief 컬럼 이름으로 컬럼 지정.
   * @param name 컬럼 이름.
   * @return 지정된 컬럼 값을 나타내는 MariaResultVal 객체.
   */
  MariaResultVal operator[](const std::string &name)
  {
    return this->operator[](name.c_str());
  }

  /**
   * @brief 컬럼 이름(C 스타일 문자열)으로 컬럼 지정.
   * @param name 컬럼 이름.
   * @return 지정된 컬럼 값을 나타내는 MariaResultVal 객체.
   */
  MariaResultVal operator[](const char *name)
  {
    column_ = rs_->findColumn(name) + 1;
    return MariaResultVal(column_ - 1, rs_);
  }

  /**
   * @brief 다음 행(row)으로 이동.
   *
   * 행 이동 시 현재 컬럼 인덱스는 1로 초기화됩니다.
   * @return 다음 행이 존재하면 true, 그렇지 않으면 false.
   */
  bool next()
  {
    if (rs_ == nullptr)
      return false;

    column_ = 1; // 새 행(row) 시작 시 리셋
    return rs_->next();
  }

  /**
   * @brief 내부 sql::ResultSet 객체를 반환.
   * @return sql::ResultSet의 포인터.
   */
  const sql::ResultSet *rs() { return rs_.get(); }

protected:
  maria_conn_sptr conn_; ///< 연결 객체를 보유하여 외부에서 해제되더라도 유지.
  std::shared_ptr<sql::ResultSet> rs_; ///< SQL 결과 셋의 공유 포인터.
  mutable int32_t column_ = 1; ///< 현재 컬럼 인덱스 (1부터 시작).
};









