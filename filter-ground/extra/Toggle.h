/*
 * Toggle.h
 *
 *  Created on: 2020. 7. 30.
 *      Author: tys
 */

#pragma once

#include <extra/SpinLockGuard.h>

class Toggle
{
public:
  // @brief 값이 오는 경우 컴파일러의 암묵적 형변환 금지함. explicit
  explicit Toggle(bool &&on        = false,
                  bool &&use_lock  = true)
  : on_(std::forward<bool>(on)), use_lock_(std::forward<bool>(use_lock)) {}

  Toggle(Toggle &&rhs)
  {
    on_       = std::move(rhs.on_);
    use_lock_ = std::move(rhs.use_lock_);
  }

  Toggle(const Toggle &rhs)
  {
    on_       = rhs.on_;
    use_lock_ = rhs.use_lock_;
  }

  // 복사시 lock 상태는 복사하지 않는다.
  void operator=(const Toggle &rhs)
  {
    on_       = rhs.on_;
    use_lock_ = rhs.use_lock_;
  }

  void operator=(Toggle &&rhs)
  {
    on_       = std::move(rhs.on_);
    use_lock_ = std::move(rhs.use_lock_);
  }

  // 이미 켜져 있으면 false, 꺼졌다 켜지면 true
  bool turn_on()
  {
    SpinLockGuard(lock_, use_lock_);
    if (on_ == true)
      return false;

    on_ = true;
    return true;
  }

  // on의 반대.
  bool turn_off()
  {
    SpinLockGuard(lock_, use_lock_);
    if (on_ == false)
      return false;

    on_ = false;
    return true;
  }

  bool is_turned_on() const
  {
    SpinLockGuard(lock_, use_lock_);
    return on_;
  }

  bool is_turned_off() const
  {
    SpinLockGuard(lock_, use_lock_);
    return !on_;
  }

private:
  bool  on_;
  mutable bool use_lock_ = true;
  mutable SpinLock lock_;
};



