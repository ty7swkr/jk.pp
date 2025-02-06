#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>

// 시그널을 놓치는 문제와 가짜깨움이 발생하지 않도록 구현.
//
// 단, 1:1 사용에서만 안전하게 사용 가능하며,
// 1:N은 정상작동하지 않음.
//
// 1:N 혹은 N:N 모델로 사용하고자 한다면 MSignal을 그룹핑하여 사용하는 방법이 있음. <--!!!!!!
//
// lost Wakeup     - wait가 아닐때 signal을 보내면 wait를 받지 못함.
// spurious Wakeup - POSIX나 모든 OS에서 signal을 줬을때 하나만 깨어나는 것이 아니라 
//                   동시에 여러 wait condition이 깨어나는 현상을 뜻합니다. 
//                   이는 OS의 성능 이슈이기 때문에 개발자 영역으로 남겨져 있습니다.
//

// example)
//  class WorkerThread : public MThread {
//  public:
//    void signal()
//    {
//      signal.notify_one();
//    }
//
//    void stop()
//    {
//      MThread::join();
//    }
//
//  protected:
//    void run() override
//    {
//      while(1)
//      {
//        signal.wait();
//        std::cout << "signal received" << std::endl;
//      }
//    }
//
//  private:
//    MSignalsignal;
//  };
//
//  int main()
//  {
//    WorkerThread worker;
//    worker.start(); // Mthread::start()
//
//    while(1)
//    {
//      worker.signal();
//      std::this_thread::sleep_for(std::chrono::seconds(1));
//    }
//
//    worker.stop();
//  }

class MSignal
{
public:
  MSignal () = default;
  ~MSignal() = default;

  // 1:N 사용에서는 시그널을 놓지는 문제로
  // notify_all은 사용하지 않음.
  // void notify_all() { cond_.notify_all(); }

  std::unique_lock<std::mutex>
  scoped_acquire_lock() const
  {
    return std::move(std::unique_lock<std::mutex>(this->lock_));
  }

  void notify_one(std::unique_lock<std::mutex> &locked_scoped_lock)
  {
    notify_one_nolock();
    locked_scoped_lock.unlock();
  }

  // false : timeout, true : wakeup
  bool wait(const uint32_t &msec = 0);

  // func이 true를 리턴하면 wait가 즉시 깨어남
  // 대기전 락을 걸고 func호출 false이면 대기 -> 깨어났을때 다시 func호출 -> 이때 fale이면 다시 대기...
  void wait(std::function<bool()> func);

  // false : timeout, true : wakeup
  bool wait(const uint32_t &msec, std::function<bool()> func);

  // lock은 자동 언락됨. 이때 첫번째 인자 lock는 scoped_acquire_lock의 락을 사용해야함.
  // false : timeout, true : wakeup
  bool wait(std::unique_lock<std::mutex> &lock, const uint32_t &msec = 0);

  void notify_one()
  {
    std::lock_guard<std::mutex> lock(lock_);
    notify_one_nolock();
  }

  // notify_one(람다) 형태 함수군
  // notify_one은 락을 걸고 람다함수를 실행하여주고 난 후 시그널을 보내고 락을 풉니다.
  // 람다함수의 형태에 따라 아래 4가지가 있습니다.
  //
  // 리턴값이 없는 notify_one
  //  void notify_one(람다함수)
  //  void notify_one(람다함수(lock))
  //
  // 람다함수의 리턴값(템플릿)을 전달해주는 notify_one, 리턴형식은 무엇이든 상관 없습니다.
  //  int notify_one(람다함수() -> int)
  //  string notify_one(람다함수(lock)->string)
  //
  //  signal.notify_one([&]()
  //                   {
  //                     // 락이 걸린 상태에서 함수가 실행되고 시그널을 보낸 후 자동으로 락이 풀립니다.
  //                     std::cout << "locked output" << std::endl;
  //                   }
  //
  //  // 임의데로 락을 제어할 수 있도록 락을 파라미터로 전달합니다.
  //  signal.notify_one([&](std::unique_lock<std::mutex> &lock)
  //                   {
  //                     // 락이 걸린 상태에서 이 함수에 진입됩니다.
  //                     std::cout << "locked output" << std::endl;
  //                     lock.unlock();
  //                     // 락이 풀린 상태에서 실행됩니다.
  //                     std::cout << "unlocked output" << std::endl;
  //                     // 이 함수가 종료된 후 락이 걸려있다면 자동 해제되며 그렇지 않다면 해제된 채로
  //                     // 시그널을 보냅니다.
  //                   }
  //
  //  리턴을 전달해주는 notify_one 함수입니다.
  //  위 notify_one가 기본 사용법은 같으나 람다에서 리턴한 값을 notify_one도 같은 형식으로 리턴하여 줍니다.
  //
  //  return signal.notify_one([&]() -> bool
  //                          {
  //                            // 락이 걸린 상태에서 함수가 실행되고 시그널을 보낸 후 자동으로 락이 풀립니다.
  //                            std::cout << "locked output" << std::endl;
  //                            return true;
  //                          }
  //
  //  임의데로 락을 제어할 수 있도록 락을 파라미터로 전달합니다.
  //  위 notify_one가 기본 사용법은 같으나 람다에서 리턴한 값을 notify_one도 같은 형식으로 리턴하여 줍니다.
  //
  //  return signal.notify_one([&](std::unique_lock<std::mutex> &lock) -> std::string
  //                           {
  //                             // 락이 걸린 상태에서 이 함수에 진입됩니다.
  //                             std::cout << "locked output" << std::endl;
  //                             lock.unlock();
  //                             // 락이 풀린 상태에서 실행됩니다.
  //                             std::cout << "unlocked output" << std::endl;
  //                             // 이 함수가 종료된 후 락이 걸려있다면 자동 해제되며 그렇지 않다면 해제된 채로
  //                             // 시그널을 보냅니다.
  //                             return "unlocked";
  //                           }

  // 기본 케이스 - 인자가 없는 람다, non-void 리턴
  template<typename F,
           typename = typename std::enable_if<!std::is_void<decltype(std::declval<F>()())>::value>::type>
  auto notify_one(F&& func) -> decltype(std::declval<F>()())
  {
    std::lock_guard<std::mutex> lock(lock_);
    auto result = std::forward<F>(func)();
    notify_one_nolock();
    return result;
  }

  // void 리턴 특수화 - 인자가 없는 람다
  //
  // 템플릿 해석은 아래와 같습니다.
  // decltype(std::declval<F>()()) - 람다의 반환 타입을 얻음
  // std::is_void<...> - 그 타입이 void인지 체크
  // std::enable_if<...>::type - void일 때만 이 특수화가 활성화되도록 함
  template<typename F,
           typename = typename std::enable_if<std::is_void<decltype(std::declval<F>()())>::value>::type,
           typename = void>
  void notify_one(F&& func)
  {
    std::lock_guard<std::mutex> lock(lock_);
    std::forward<F>(func)();
    notify_one_nolock();
  }

  // unique_lock을 인자로 받는 람다 특수화, non-void 리턴
  template<typename F,
           typename = typename std::enable_if<!std::is_void<decltype(std::declval<F>()(std::declval<std::unique_lock<std::mutex>&>()))>::value>::type,
           typename = void,
           typename = void>
  auto notify_one(F&& func) -> decltype(std::declval<F>()(std::declval<std::unique_lock<std::mutex>&>()))
  {
    std::unique_lock<std::mutex> lock(lock_);
    auto result = std::forward<F>(func)(lock);
    notify_one_nolock();
    return result;
  }

  // void 리턴 특수화 - unique_lock을 인자로 받는 람다
  template<typename F,
           typename = typename std::enable_if<std::is_void<decltype(std::declval<F>()(std::declval<std::unique_lock<std::mutex>&>()))>::value>::type,
           typename = void, // 더미파라미터
           typename = void,
           typename = void>
  void notify_one(F&& func)
  {
    std::unique_lock<std::mutex> lock(lock_);
    std::forward<F>(func)(lock);
    notify_one_nolock();
  }

protected:
  void notify_one_nolock()
  {
    if (signaled_ == true)
      return;

    signaled_ = true;
    cond_.notify_one();
  }

private:
  mutable std::mutex      lock_;
  std::condition_variable cond_;
  bool                    signaled_ = false;
};

inline bool
MSignal::wait(const uint32_t &msec)
{
  std::function<bool()> pred = [&]()
  {
    if (signaled_ == true)
    {
      signaled_ = false;
      return true;
    }
    return false;
  };

  std::unique_lock<std::mutex> lock(lock_);
  if (msec == 0)
  {
    cond_.wait(lock, pred);
    return true;
  }

  std::chrono::milliseconds timeout(msec);
  return cond_.wait_for(lock, timeout, pred);
}

inline bool
MSignal::wait(const uint32_t &msec, std::function<bool()> func)
{
  size_t count = 0;
  std::function<bool()> pred = [&]()
  {
    decltype(signaled_) signaled = signaled_;

    if (signaled_ == true)
      signaled_ = false;

    if (func != nullptr)
    {
      if (count++ ==     0) return func(); // 최초.
      if (signaled == true) return func();
    }

    return false;
  };

  std::unique_lock<std::mutex> lock(lock_);
  if (msec == 0)
  {
    cond_.wait(lock, pred);
    return true;
  }

  std::chrono::milliseconds timeout(msec);
  return cond_.wait_for(lock, timeout, pred);
}

inline void
MSignal::wait(std::function<bool()> func)
{
  size_t count = 0;
  std::function<bool()> pred = [&]()
  {
    decltype(signaled_) signaled = signaled_;

    if (signaled_ == true)
      signaled_ = false;

    if (func != nullptr)
    {
      if (count++ ==     0) return func(); // 최초.
      if (signaled == true) return func();
    }

    return signaled;
  };

  std::unique_lock<std::mutex> lock(lock_);
  cond_.wait(lock, pred);
}

inline bool
MSignal::wait(std::unique_lock<std::mutex> &lock, const uint32_t &msec)
{
  std::function<bool()> pred = [&]()
  {
    if (signaled_ == true)
    {
      signaled_ = false;
      return true;
    }
    return false;
  };

  if (msec == 0)
  {
    cond_.wait(lock, pred);
    return true;
  }

  std::chrono::milliseconds timeout(msec);
  return cond_.wait_for(lock, timeout, pred);
}






