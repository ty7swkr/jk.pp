/*
 * StopWaiter.h
 *
 *  Created on: 2024. 1. 28.
 *      Author: tys
 */

#pragma once

#include <extra/MSignal.h>

class StopWaiter
{
public:
  void stop()
  {
    signal_.notify_one([&]()
    {
      if (stop_ == true)
        return;

      stop_ = true;
    });
  }

  // false : timeout
  bool wait(const size_t &msec = 0) // 0:무한대기
  {
    // f 리턴값이 true면 wait에서 빠져나온다. / f 리턴값이 false면 락을 풀고 wait 상태로 진입
    return signal_.wait(msec, [&]() { return stop_; });
  }

private:
  bool    stop_ = false;
  MSignal signal_;
};
