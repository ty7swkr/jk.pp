/*
 * StreamLoggerWriter.h
 *
 *  Created on: 2024. 12. 13.
 *      Author: tys
 */

#pragma once

#include <queueable.h>

#include <stream_logger/StreamLoggerData.h>
#include <extra/LockFreeQueueThread.h>
#include <extra/BlockingVectorThread.h>
#include <extra/Singleton.h>
#include <string>
#include <memory>
#include <iostream>

/**
 * @brief StreamLoggerWriter
 * @details
 * - StreamLoggerWriter는 로그 데이터를 출력하는 쓰레드 클래스입니다.
 * - StreamLoggerData를 받아서 사용자 정의 로그 출력 함수를 호출하거나 표준 출력합니다.
 * - 사용자 정의 로그 출력 함수가 false를 리턴하면 표준 출력하지 않습니다.
 * - 사용자 정의 로그 출력 함수가 없으면 표준 출력합니다.
 */
class StreamLoggerWriter : public LockFreeQueueThread<false, queueable_t<StreamLoggerData>, boost::lockfree::fixed_sized<true>>
{
public:
  StreamLoggerWriter() : LockFreeQueueThread(10000) {}

  std::function<bool(const StreamLoggerData &data)> user_log_func = nullptr;

  // 0 : 성공
  // -1 : 큐닫힘
  // 양수 큐 꽉참. EAGAIN
  int push(std::shared_ptr<StreamLoggerData> item)
  {
    queueable_t<StreamLoggerData> queue_item(item);
    static const auto handle_return = [](int result, queueable_t<StreamLoggerData> &queue_item) -> int
    {
      if (result != 0) queue_item.destroy();
      return result;
    };
    return handle_return(waiter_.push(queue_item), queue_item);
  }

protected:
  void run() override
  {
    queueable_t<StreamLoggerData> item;
    while (waiter_.pop(item) >= 0)
    {
      auto data = item.take();

      if (user_log_func != nullptr)
        if (user_log_func(*(data.get())) == false)
          continue;

      /// 요구사항이 표준출력임.
      std::cout << data->to_json() << std::endl;
    }
  }
};








