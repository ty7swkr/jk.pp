/*
 * Worker.h
 *
 *  Created on: 2024. 12. 2.
 *      Author: tys
 */

#pragma once

#include <queueable.h>

#include <extra/LockFreeQueueThread.h>
#include <extra/SysDateTimeDiff.h>
#include <extra/helper.h>

/**
 * @class Worker
 * @brief 작업을 처리하는 기본 워커 클래스
 * @tparam WOKER_RECV_TYPE 워커가 수신하는 데이터 타입
 * @tparam POOL_PUSH_TYPE 큐풀에서 보낼 데이터 타입
 * @details
 * 락프리 큐를 사용하여 스레드 안전하게 작업을 처리하는 워커 클래스입니다.
 * 큐잉된 작업을 순차적으로 처리하며, 재시도 메커니즘을 지원합니다.
 * 푸시되는 데이터 형식과 큐에 전송될 데이터 형식을 다르게 할 수 있습니다.
 * 사용자는 virtual int push(const POOL_PUSH_TYPE &message) 를 작성해 주어야 합니다.
 */
template<typename WOKER_RECV_TYPE, typename POOL_PUSH_TYPE = std::pair<std::string, std::string>>
class Worker : public LockFreeQueueThread<false, queueable_t<WOKER_RECV_TYPE>, boost::lockfree::fixed_sized<true>>
{
public:
  using Base = LockFreeQueueThread<false, queueable_t<WOKER_RECV_TYPE>, boost::lockfree::fixed_sized<true>>;
  using Base::waiter_;  // 부모 클래스의 waiter_를 사용하겠다고 명시적으로 선언

  /**
   * @brief 생성자
   * @param queue_size 작업 큐의 크기 (기본값: 10000)
   * @details 지정된 크기의 락프리 큐를 가진 워커를 생성합니다.
   */
  Worker(const size_t &queue_size = 10000)
  : LockFreeQueueThread<false, queueable_t<WOKER_RECV_TYPE>, boost::lockfree::fixed_sized<true>>(queue_size) {}
  virtual ~Worker() {}

  /// POOL_PUSH_TYPE형태를 가공해서 WOKER_RECV_TYPE으로 변환하여 처리할 책임이 있다.
  /// 내부 try_push를 호출해야 한다.
  virtual int push(const POOL_PUSH_TYPE &message) = 0;

  void assigned_no(const size_t &no) { assigned_no_ = no; }

protected:
  /**
   * @brief 작업 항목을 큐에 추가 (재시도 지원)
   * @param item 추가할 작업 항목
   * @param max_retries 최대 재시도 횟수
   * @return int 작업 추가 결과
   * @retval 0 성공
   * @retval -1 큐가 닫힘
   * @retval EAGAIN 재시도 횟수를 모두 소진함
   * @details
   * - max_retries가 0인 경우 한 번만 시도합니다.
   * - 지정된 재시도 횟수만큼 작업 추가를 시도합니다.
   * - 큐가 닫힌 경우(-1) 즉시 반환합니다.
   * - 성공(0)한 경우 즉시 반환합니다.
   * - 모든 재시도가 실패하면 EAGAIN을 반환합니다.
   */
  virtual int try_push(const WOKER_RECV_TYPE &item, const size_t &max_retries)
  {
    queueable_t<WOKER_RECV_TYPE> queue_item(item);

    // 결과를 처리하고 리턴하는 람다 함수
    // push 결과가 0이면 destroy하지 않고, 그 외에는 destroy 후 결과값 반환
    static const auto handle_return = [](int result, queueable_t<WOKER_RECV_TYPE> &queue_item) -> int
    {
      if (result != 0) queue_item.destroy();
      return result;
    };

    if (max_retries == 0)
      return handle_return(waiter_.push(queue_item), queue_item);

    int res = 0;
    for (size_t index = 0; index < max_retries; ++index)
      if ((res = waiter_.push(queue_item)) <= 0)
        return handle_return(res, queue_item);

    return handle_return(EAGAIN, queue_item);
  }

  std::string assigned_no_str() const { return to_stringf(assigned_no_, "%02d"); }

protected:
  /**
   * @brief 워커의 메인 실행 함수
   * @details
   * 상속받은 클래스에서 반드시 구현해야 하는 순수 가상 함수입니다.
   * 실제 작업 처리 로직을 이 함수에 구현합니다.
   */
  virtual void run() override = 0;
  size_t assigned_no_ = 0;
};


