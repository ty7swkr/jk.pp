/*
 * FilterWorker.h
 *
 *  Created on: 2024. 12. 17.
 *      Author: tys
 */

#pragma once

#include <NatsSenders.h>
#include <AppConf.h>
#include <Worker.h>
#include <FilterTpsMeter.h>
#include <filter_info.h>
#include <extra/SysDateTime.h>
#include <extra/Toggle.h>
#include <extra/Optional.h>

/**
 * @brief 필터링 작업을 수행하는 워커 기본 클래스
 * @details 메시지 필터링, 폐기 처리, NATS 결과 전송 등의 기본 기능 제공
 *
 * 템플릿 인자 설명.
 * std::tuple<std::string, filter_info_t, SysDateTime> : Worker::waiter_(LockFreeQueue)에서 사용하는 데이터 형식
 *    : subject, filter_01_t, 수신시간
 * std::pair<std::string, std::string>> : NATS로부터 받은 메세지 형식
 *    : subject, JSON 형식의 메시지 문자열
 *
 * NATS로부터 받은 메세지 형식을 Worker(LockFreeQueueThread)에서 사용하는 데이터 형식으로 변환하여
 * push에서 전달하면 Worker::waiter_에 데이터가 쌓이게 된다.
 *
 * FilterWorker를 상속받은 클래스는(ex. XxxFilterWorker)
 * XxxFilterWorker::run()함수를 구현할때 Worker::waiter_에서 데이터를 꺼내서 처리해야 한다.
 */
/// template<typename WOKER_RECV_TYPE, typename POOL_PUSH_TYPE = std::pair<std::string, std::string>>
class FilterWorker : public Worker<std::tuple<std::string, filter_info_t, SysDateTime>, std::pair<std::string, std::string>> ///< subject, filter_01_t, 수신시간
{
public:
  /**
   * @brief 워커 생성자
   * @param queue_size 작업 큐의 최대 크기 (기본값: 10000)
   */
  FilterWorker(const size_t &queue_size = 10000)
  : Worker(queue_size) , error_toggle_(false, false) {}

  /**
   * @brief 메시지를 워커 큐에 추가
   * @param message subject, JSON 형식의 메시지 문자열
   * @return 큐 추가 결과 코드
   */
  int push(const std::pair<std::string, std::string> &message) override;

protected:
  /**
   * @brief 워커 메인 실행 함수 (하위 클래스에서 구현)
   */
  virtual void handle_filter(filter_info_t &filter, const std::string &subject, const SysDateTime &recv_time)
  { (void)filter; (void)subject; (void)recv_time; }

  /**
   * @brief 애플리케이션 설정 정보 획득 (하위 클래스에서 구현)
   */
  virtual const AppConf &get_app_conf() const = 0;

protected:
  virtual void run() override;

  /**
   * @brief 필터링 시작/종료 시간 설정
   */
  virtual void set_filtering_time(filtering_time_objs &filtering_times, const SysDateTime &recv_time) const;

protected:
  /**
   * @brief JSON 메시지를 필터 정보 객체로 파싱
   * @return 파싱된 필터 정보 (Optional)
   */
  Optional<filter_info_t>
       parse_message      (const std::string &message)  const;

  /**
   * @brief 필터링 결과를 NATS로 전송
   * @param smpp_result SMPP 결과 코드
   * @param result_code 처리 결과 코드
   * @param reason_code 사유 코드
   */
  void to_result_nats     (filter_info_t &filter, const SysDateTime &recv_time,
                           const int &smpp_result, const int &result_code, const int &reason_code) const;

  /**
   * @brief 필터링 결과를 NATS로 전송
   * @param smpp_result SMPP 결과 코드
   * @param result_code 처리 결과 코드
   * @param reason_code 사유 코드
   */
  void to_next_nats       (filter_info_t &filter, const SysDateTime &recv_time) const;

  void to_nats_result     (filter_info_t &filter, const SysDateTime &recv_time,
                           const int &smpp_result, const int &result_code, const int &reason_code) const
  {
    to_result_nats(filter, recv_time, smpp_result, result_code, reason_code);
  }

  /**
   * @brief 메시지 폐기 여부 확인 및 처리
   * @return 폐기 처리 여부
   */
  virtual bool handle_discard     (filter_info_t &filter, const SysDateTime &recv_time) const;

private:
  /**
   * @brief 큐 가득 참 상태 폐기 처리
   */
  virtual bool discard_queue_full (filter_info_t &filter, const SysDateTime &recv_time) const;

  /**
   * @brief 입력 TPS 초과 폐기 처리
   */
  virtual bool discard_tps_in     (filter_info_t &filter, const SysDateTime &recv_time) const;

  /**
   * @brief 출력 TPS 초과 폐기 처리
   */
  virtual bool discard_tps_out    (filter_info_t &filter, const SysDateTime &recv_time) const;

  /**
   * @brief 처리 시간 초과 폐기 처리
   */
  virtual bool discard_timeout    (filter_info_t &filter, const SysDateTime &recv_time) const;

private:
  mutable bool   handle_discard_ = false;
  mutable Toggle error_toggle_;
};
