#pragma once

#include <extra/BlockingLockFreeQueue.h>
#include <extra/MThread.h>
#include <boost/lockfree/queue.hpp>

/**
 * @class LockFreeQueueThread
 * @brief boost::lockfree::queue 를 사용하는 쓰레드 기반 클래스.
 * @details
 * @tparam SIGNALED, 큐에서 처리할 데이터 타입. false는 스피닝기반, true는 signal기반
 * @tparam T, 큐에서 처리할 데이터 타입.(예제에서 int)
 * @tparam OPTIONS, boost::lockfree::fixed_sized<true>등 boost옵션처리 인자.
 * 고정 크기의 Lock-Free 큐를 기반으로 동작하는 쓰레드 클래스입니다.
 * 쓰레드의 시작, 종료, 실행 로직을 관리하며, 비즈니스 로직은 `run()` 메서드에서 구현합니다.
 * 템플릿 인자로 스피닝 기반의 동작과 SGINAL기반의 동작 두가지중 선택할 수 있습니다.
 *
 * 스피닝기반 : 속도가 빠르나 대기시 cpu점유율 5~10% 내외
 * 시그널기반 : 속도가 느리나 대기시 cpu점유율 0%
 *
 * 아래 예제처럼 push는 직접 구현해야 합니다. 주의해야할 점은 push시 EAGAIN을 리턴하면 큐가 꽉 찬겁니다.
 * 각자 구현에 맞게 처리해야 합니다.
 *
 * @see BlockingLockFreeQueue
 *
 * example
class MyThread : public LockFreeQueueThread<false, int>
{
public:
  bool push(const int &item) { return waiter_.push(item); }

protected:
  void run() override
  {
    int data;
    while (waiter_.pop(data) == 0)
    {
      // 비지니스 처리
      d_log << data;
    }
    // 종료처리.
  }
};

int main()
{
  MyThread thread;
  thread.start();
  while (true)
  {
    thread.push(1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}
 */
template<bool SIGNALED, typename T, typename... Options>
class LockFreeQueueThread : public MThread
{
public:
  /**
   * @brief LockFreeQueueThread 생성자.
   * @details 주의 큐는 시작시 close한 상태로 시작함.(open = false), start시 open됨.
   * @param capacity 큐의 고정 크기.
   */
  LockFreeQueueThread(const size_t &capacity = 10000) : waiter_(capacity, false) {}
  virtual ~LockFreeQueueThread() {}

          bool start() override;
  virtual bool stop ();
  virtual void run  () = 0;

  /// 락프리큐 특성상 size는 정확하지 않을 수 있습니다. 음수일 수 도 있습니다.
  int64_t      size () const { return waiter_.size(); }

protected:
  BlockingLockFreeQueue<SIGNALED, T, Options...> waiter_; ///< Lock-Free 큐 객체.
};

/**
 * @brief 쓰레드를 시작합니다.
 * @return 성공 시 true, 실패 시 false.
 */
template<bool SIGNALED, typename T, typename... Options> bool
LockFreeQueueThread<SIGNALED, T, Options...>::start()
{
  if (waiter_.is_open() == true)
    return true;

  waiter_.open();
  return MThread::start();
}

/**
 * @brief Lock-Free 큐를 닫고, 쓰레드를 종료합니다.
 * @return 성공 시 true, 실패 시 false.
 */
template<bool SIGNALED, typename T, typename... Options> bool
LockFreeQueueThread<SIGNALED, T, Options...>::stop()
{
  if (waiter_.is_open() == false)
    return true;

  waiter_.close();
  return MThread::join();
}


