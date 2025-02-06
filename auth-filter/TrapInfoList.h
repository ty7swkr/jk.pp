/*
 * SmishingUrl.h
 *
 *  Created on: 2024. 12. 5.
 *      Author: tys
 */

#pragma once

#include <CnapsDB.h>
#include <extra/BlockingDequeThread.h>
#include <extra/Singleton.h>

#include <unordered_set>

#define trap_info_list TrapInfoList::ref()

/**
 * @class TrapInfoList
 * @brief 트랩 사용자 정보
 */
class TrapInfoList : public BlockingDequeThread<>,
                     public Singleton<TrapInfoList>
{
public:
  /**
   * @brief 저장소를 초기화하고 모니터링 스레드를 시작합니다
   * @return 초기화 및 스레드 시작 성공 시 true, 실패 시 false 반환
   */
  bool start();

  /**
   * @brief 주어진 URL이 악성 URL 저장소에 존재하는지 확인합니다
   * @param url 검사할 URL
   * @return 저장소에 URL이 존재하면 true, 아니면 false 반환
   * @thread_safety 이 메서드는 스레드 세이프합니다
   */
  bool contains(const std::string &cust_number) const
  {
    std::lock_guard<std::mutex> guard(lock_);
    return numbers_.count(cust_number) > 0;
  }

  template<size_t N> bool
  contains(const char (&value)[N]) const
  {
    return this->contains(std::string(value, N-1));
  }

protected:
  /**
   * @brief 백그라운드 스레드 실행 함수
   *
   * app_conf.table_check_period_ms 간격으로 데이터베이스 업데이트를 확인하는 루프를 실행합니다
   */
  void run() override;

  /**
   * @brief 데이터베이스 내용이 변경된 경우 URL 저장소를 업데이트합니다
   * @return 업데이트가 수행된 경우 true, 업데이트가 필요없거나 오류 발생시 false 반환
   */
  bool update_container();

protected:
  mutable std::mutex              lock_;          ///< numbers__ 접근을 위한 뮤텍스
  std::unordered_set<std::string> numbers_;       ///< trap_info 세트
  std::atomic<int64_t>            checksum_{-1};  ///< 현재 데이터베이스 체크섬
};



