#pragma once

#include <extra/MSignal.h>
#include <boost/lockfree/queue.hpp>
#include <thread>

/**
 * @class BlockingLockFreeQueue
 * @brief boost::lockfree::queue 를 사용하는 블럭킹 큐 클래스.
 * @tparam SIGNALED, 큐에서 처리할 데이터 타입. false는 스피닝기반, true는 signal기반
 * @tparam T, 큐에서 처리할 데이터 타입.
 * @tparam Options, boost::lockfree::queue의 옵션
 * @details
 * 기본동작
 *    블럭킹 큐는 큐가 비어있을때 pop을 호출하면 대기하다가 큐에 데이터가 들어오면 리턴합니다.
 *    큐는 기본값으로 open이 true로 시작합니다.
 * 큐 닫기
 *    큐를 닫으려면 close를 호출하면 됩니다. 큐가 닫히면 pop을 호출하면 -1을 리턴합니다.
 *    이때 모든 큐에 남아있는 데이터가 pop된 후 -1을 리턴합니다.
 * push 동작
 *    큐가 꽉차면 push를 호출해도 EAGAIN을 리턴합니다. 보통 while문으로 처리합니다.
 * pop 동작
 *    pop은 adaptive하게 동작합니다. 큐가 비어있으면 spin하다가 일정시간이 지나면 sleep합니다.
 */
template<bool SIGNALED, typename T, typename... Options>
class BlockingLockFreeQueue
{
public:
  BlockingLockFreeQueue(const size_t &capacity = 10000, const bool &open = true)
  : queue_(capacity), open_(open) {}

  ~BlockingLockFreeQueue() {}

  bool is_signaled() const { return SIGNALED; }

  void open()
  {
    open_ = true;
  }

  void close()
  {
    open_ = false;
    if (SIGNALED == true)
      signal_.notify_one();
  }

  // 0 : 성공
  // -1 : 큐닫힘
  // 양수 큐 꽉참. EAGAIN
  int push(const T &item)
  {
    if (open_.load() == false)
      return -1;

    if (queue_.push(item) == false)
      return EAGAIN;

    ++size_;

    if (SIGNALED == true)
      if (queue_.empty() == false)
        signal_.notify_one();

    return 0;
  }

  // 0 : 정상수신
  // -1 : 큐 닫힘
  // 양수 : 타임아웃(ETIMEDOUT = 110) <- 확장 예정.
  // condition variable의 특성상 발생할 수 있는 "lost wakeup" 또는 "missed notification" 문제
  // 는 MSignal 클래스에서 처리되므로 상관이 없다.
  int pop(T &item, const int64_t &timeout_ms = 0)
  {
    if (SIGNALED == true)
      return signaled_pop(item);

    // ADAPTIVE SPIN처리
    return adaptive_pop(item, timeout_ms);
  }

  bool empty() const
  {
    return queue_.empty();
  }

  // lockfree 특성상 음수가 발생할 수도 있습니다.
  int64_t size() const
  {
    return size_.load();
  }

  size_t capacity() const
  {
    return queue_.capacity();
  }

  bool is_open() const
  {
    return open_.load();
  }

protected:
  int signaled_pop(T &item)
  {
    while (true)
    {
      if (queue_.pop(item) == true)
      {
        --size_;
        return 0;
      }

      if (queue_.empty() == true && open_ == false)
        return -1;

      signal_.wait();
    }

    return 0;
  }

  // 0 : 정상수신
  // -1 : 큐 닫힘
  int adaptive_pop(T & item, const int64_t &timeout_ms = 0)
  {
     int fails = 0;

     auto sta = std::chrono::steady_clock::now();
     while (queue_.pop(item) == false)
     {
       if (open_.load() == false)
         return -1;

       // timeout_ms가 0보다 큰 경우에만 타임아웃 체크
       if (timeout_ms > 0)
       {
         auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                         (std::chrono::steady_clock::now() - sta).count();
         if (elapsed >= timeout_ms)
           return ETIMEDOUT;  // 타임아웃 발생
       }

       if (fails < 1000)
         fails++;
       else
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
     }

     --size_;
     return 0;
  }

private:
  boost::lockfree::queue<T, Options...> queue_;
  MSignal signal_;
  std::atomic<bool> open_{false};
  std::atomic<int64_t> size_{0};
};

