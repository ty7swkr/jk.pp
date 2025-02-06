/*
 * SpinLockGuard.h
 *
 *  Created on: 2019. 10. 24.
 *      Author: tys
 */

#pragma once

#include <memory>
#include <atomic>
#include <algorithm>

/**
 * context switching에 빠지지 않고 락을 가져온다.
 * 단 쓰레드 수가 코어수보다 적으면 context switching이 더 많이 발생하므로 주의해야함.
 */

class SpinLock;

// You must use SharedPtrSpinLock to copy or share a SpinLock.
using SpinLockPtr = std::shared_ptr<SpinLock>;

class SpinLock
{
public:
  SpinLock()  noexcept = default;
  ~SpinLock() noexcept = default;

  // 실제 copy는 없음 호환성때문에 함수만 만듦.
  SpinLock(const SpinLock &l) { (void)l; };
  SpinLock& operator=(const SpinLock &l) { (void)l; return *this; };
//  SpinLock(const SpinLock &l) = delete;
//  SpinLock& operator=(const SpinLock &l) = delete;

  void lock()
  {
    while(spin_lock_.test_and_set(std::memory_order_acquire)) {}
    ++lock_count_;
    return;
  }

  void unlock()
  {
    --lock_count_;
    spin_lock_.clear(std::memory_order_release);
  }

  bool is_lock() const
  {
    return lock_count_ > 0;
  }

private:
  std::atomic_flag      spin_lock_  = ATOMIC_FLAG_INIT;
  std::atomic<int32_t>  lock_count_{0};

//  friend void lock  (SpinLock &lhs, SpinLock &rhs);
//  friend void unlock(SpinLock &lhs, SpinLock &rhs);
};

inline void
lock_both(SpinLock &a, SpinLock &b)
{
  SpinLock *first  = &a;
  SpinLock *second = &b;

  if (std::less<SpinLock*>()(&b, &a))
    std::swap(first, second);

  first->lock();
  second->lock();
}

inline void
unlock_both(SpinLock &a, SpinLock &b)
{
  a.unlock();
  b.unlock();
}

//inline void
//lock(SpinLock &lhs, SpinLock &rhs)
//{
//  SpinLock *lock1 = &lhs;
//  SpinLock *lock2 = &rhs;
//
//  if (&lhs > &rhs)
//  {
//    lock1 = &rhs;
//    lock2 = &lhs;
//  }
//
//  while (true)
//  {
//      // Test and set lock1 바쁜 대기, spin
//    while (lock1->spin_lock_.test_and_set(std::memory_order_acquire)) {}
//    ++(lock1->lock_count_);
//
//    // Test and set lock2
//    if (!lock2->spin_lock_.test_and_set(std::memory_order_acquire))
//    {
//      ++(lock2->lock_count_);
//      break; // 두 lock 모두 획득
//    }
//
//    // lock2를 얻지 못했으면 lock1을 해제하고 다시 시도
//    lock1->spin_lock_.clear(std::memory_order_release);
//    --(lock1->lock_count_);
//  }
//}
//
//void
//unlock(SpinLock &lhs, SpinLock &rhs)
//{
//  if (lhs.is_lock() == true) lhs.unlock();
//  if (rhs.is_lock() == true) rhs.unlock();
//}
