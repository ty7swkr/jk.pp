/*
 * WorkerPool.h
 *
 *  Created on: 2024. 12. 2.
 *      Author: tys
 */

#pragma once

#include <extra/SysDateTime.h>
#include <string>
#include <deque>

//+-----------------+
//|  Worker Pool    |
//|                 |
//|  Worker 1       |
//|  Worker 2       |
//|  Worker 3       |
//+-----------------+

/**
 * @class WorkerPool
 * @brief 워커들을 관리하고 작업을 분배하는 풀 클래스
 * @tparam WORKER 작업을 처리할 워커 클래스 타입
 * @details
 * 여러 워커 인스턴스를 관리하고 입력된 작업을 워커들에게 분배합니다.
 * Least Loaded 방식을 사용하여 부하가 가장 적은 워커에게 작업을 할당합니다.
 */
template<typename WORKER, typename POOL_PUSH_TYPE = std::pair<std::string, std::string>>
class WorkerPool
{
public:
  /**
   * @brief 워커 풀의 워커 수와 각 워커의 큐 크기를 설정
   * @param num 생성할 워커의 수 (기본값: 1)
   * @param queue_size 각 워커의 큐 크기 (기본값: 10000)
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   * @details
   * 지정된 수만큼 워커를 생성하고 각 워커의 큐 크기를 설정합니다.
   * 기존에 워커가 있다면 추가로 생성됩니다.
   */
  WorkerPool &set_num_of_workers(const size_t &num = 1, const size_t &queue_size = 10000)
  {
    for (size_t index = 0; index < num; ++index)
    {
      workers_.emplace_back(queue_size);
      workers_.back().assigned_no(index+1);
    }
    return *this;
  }

  /**
   * @brief 작업을 워커 풀에 추가
   * @param message 처리할 메시지
   * @return int 작업 추가 결과
   * @retval -1 워커가 없는 경우
   * @retval 그 외 선택된 워커의 push 함수 반환값
   * @details
   * Least Loaded 방식으로 현재 큐의 크기가 가장 작은 워커를 선택하여
   * 해당 워커에게 작업을 할당합니다.
   * 주석 처리된 round-robin 방식의 코드도 참고용으로 포함되어 있습니다.
   */
  int push(const POOL_PUSH_TYPE &message)
  {
    // Least Loaded
    return workers_.empty() ? -1 :
        std14_min_element(workers_.begin(), workers_.end(),
                         [](WORKER &a, WORKER &b)
                         { return a.size() < b.size(); })->push(message);
    // round robin
    // return workers_[sequence_++ % workers_.size()].push(message);
  }

  /**
   * @brief 모든 워커를 시작
   * @details 풀 내의 모든 워커의 start() 함수를 호출하여 작업 처리를 시작합니다.
   */
  void start()
  {
    for (auto &worker : workers_)
      worker.start();
  }

  /**
   * @brief 모든 워커를 중지
   * @details 풀 내의 모든 워커의 stop() 함수를 호출하여 작업 처리를 중지합니다.
   */
  void stop()
  {
    for (auto &worker : workers_)
      worker.stop();
  }

protected:
  std::deque<WORKER> workers_;  ///< 워커 인스턴스들을 저장하는 컨테이너
  //size_t sequence_ = 0;       ///< Round-robin 방식 사용 시의 시퀀스 번호 (현재 미사용)
};

