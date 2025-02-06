/**
 * @file TrapInfoList.cpp
 * @brief 트랩정보 컨테이너 구현부
 * @author tys
 */

#include "TrapInfoList.h"
#include "AuthFilterConf.h"
#include <extra/Optional.h>

/**
 * @brief 데이터베이스에서 목록을 로드합니다.
 * @return 로드된  세트를 Optional로 감싸서 반환. 실패시 nullopt 반환
 */
static Optional<std::unordered_set<std::string>>
load_data_from_db()
{
  std::unordered_set<std::string> numbers;
  try
  {
    // 목록을 조회하는 쿼리 실행
    // ASIS tm_trp_cust
    MariaStatement stmt(cnaps_db, "SELECT xx FROM xxxx");

    MariaResultSet rs = stmt.execute_query(10000);
    while (rs.next() == true)
      numbers.insert(rs["cust_num"].as_str());
  }
  catch (sql::SQLException &e)
  {
    // DB 오류 발생시 로그 기록 후 nullopt 반환
    ap_log.error() << e.getErrorCode() << ":" << e.what() << ":" << e.getSQLState();
    return nullopt;
  }

  return numbers;
}

/**
 * @brief 테이블의 현재 체크섬을 조회합니다.
 * @return 테이블 체크섬값. 오류 발생시 -1 반환
 */
static int64_t
get_checksum()
{
  try
  {
    // 테이블의 체크섬을 계산하는 쿼리 실행
    MariaStatement  stmt(cnaps_db, "CHECKSUM TABLE xxxx");
    MariaResultSet  rs = stmt.execute_query();

    while (rs.next() == true)
      return rs["Checksum"].as_int64();
  }
  catch (sql::SQLException &e)
  {
    // DB 오류 발생시 로그 기록 후 -1 반환
    ap_log.error() << e.getErrorCode() << ":" << e.what() << ":" << e.getSQLState();
  }

  return -1;
}

/**
 * @brief URL 목록을 최신 상태로 업데이트합니다.
 * @details
 * 1. 현재 테이블의 체크섬을 확인
 * 2. 체크섬이 변경된 경우에만 새로운 데이터를 로드
 * 3. 체크섬과 URL 목록을 원자적으로 업데이트
 *
 * @return 업데이트 성공시 true, 실패 또는 업데이트가 필요없는 경우 false
 */
bool
TrapInfoList::update_container()
{
  // 체크섬 확인
  int64_t checksum = get_checksum();
  if (checksum < 0 || checksum == checksum_.load())
    return false;

  // 새로운 URL 목록 로드
  auto numbers = load_data_from_db();
  if (numbers == nullopt)
    return false;

  // 체크섬과 URL 목록 업데이트
  checksum_ = checksum;
  std::lock_guard<std::mutex> guard(lock_);
  numbers_.swap(numbers.value());

  return true;
}

/**
 * @brief 저장소 시작 및 초기화
 * @return 초기화 성공시 true, 실패시 false
 */
bool
TrapInfoList::start()
{
  if (update_container() == false)
    return false;

  return BlockingDequeThread::start();
}

/**
 * @brief 백그라운드 스레드 실행 함수
 * @details 1초 간격으로 URL 목록 업데이트를 확인하는 루프를 실행합니다.
 */
void
TrapInfoList::run()
{
  update_container();

  int dummy = 0;
  while (waiter_.pop_back(dummy, app_conf.table_check_period_ms.load()) >= 0)
  {
    update_container();
  }
}













