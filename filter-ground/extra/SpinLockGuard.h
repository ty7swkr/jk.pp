/*
 * SpinLockGuard.h
 *
 *  Created on: 2019. 10. 24.
 *      Author: tys
 */

#pragma once

#include <extra/SpinLock.h>

/**
 * 멀티코어에서는 빠르다.(core개수 == thread개수인 경우)
 * 1코에 4,5쓰레드 이상에서는 mutex보다 느리다.
 *
 * 중복해서 release가 일어나지 않도록 했다.
 */
class SpinLockGuard
{
public:
  SpinLockGuard(SpinLock &lock, const bool &using_lock = true)
  : lock_(&lock), using_lock_(using_lock)
  {
    acquire();
  }

  SpinLockGuard(SpinLockPtr lock, const bool &using_lock = true)
  : lock_ptr_(lock), using_lock_(using_lock)
  {
    acquire();
  }

  ~SpinLockGuard()
  {
    release();
  }

  void acquire()
  {
    if (using_lock_ == false)
      return;

    SpinLock *lock = lock_;
    if (lock_ptr_ != nullptr)
      lock = lock_ptr_.get();

    lock->lock();
    locked_ = true;
  }

  void release()
  {
    if (using_lock_ == false)
      return;

    if (locked_ == false)
      return;

    SpinLock *lock = lock_;
    if (lock_ptr_ != nullptr)
      lock = lock_ptr_.get();

    locked_ = false;
    lock->unlock();
  }

private:
  SpinLockPtr lock_ptr_;
  SpinLock    *lock_      = nullptr;
  bool        locked_     = false;
  bool        using_lock_ = true;
};
