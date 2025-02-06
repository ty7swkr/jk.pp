/*
 * LockedDeque.h
 *
 *  Created on: 2021. 11. 16.
 *      Author: tys
 */

#pragma once

#include <extra/helper.h>
#include <extra/SpinLockGuard.h>
#include <deque>
#include <functional>

template<typename T, typename ALLOC = std::allocator<T>>
class LockedDeque
{
public:
  using container_type = std::deque<T, ALLOC>;

  LockedDeque() {}
  LockedDeque(const container_type  &container) { container_ = container; }
  LockedDeque(      container_type &&container) { container_ = std::move(container); }

  container_type
  container() const
  {
    SpinLockGuard guard(lock_);
    return container_;
  }

  void
  container(std::function<void(container_type &container)> locked_func)
  {
    SpinLockGuard guard(lock_);
    locked_func(container_);
  }

  void
  container(std::function<void(const container_type &container)> locked_func) const
  {
    SpinLockGuard guard(lock_);
    locked_func(container_);
  }

  LockedDeque<T, ALLOC> &
  operator=(const container_type &container)
  {
    SpinLockGuard guard(lock_);
    container_ = container;
    return *this;
  }

  LockedDeque<T, ALLOC> &
  append(const container_type &container)
  {
    SpinLockGuard guard(lock_);
    container_.insert(container_.end(), container.begin(), container.end());
    return *this;
  }

//  optional_ref<const T>
//  front_opt() const
//  {
//    SpinLockGuard guard(lock_);
//    if (container_.size() == 0) return std::nullopt;
//    return container_.front();
//  }
//
//  optional_ref<const T>
//  back_opt() const
//  {
//    SpinLockGuard guard(lock_);
//    if (container_.size() == 0) return std::nullopt;
//    return container_.back();
//  }
//
//  optional_ref<T>
//  front_opt()
//  {
//    SpinLockGuard guard(lock_);
//    if (container_.size() == 0) return std::nullopt;
//    return container_.front();
//  }
//
//  optional_ref<T>
//  back_opt()
//  {
//    SpinLockGuard guard(lock_);
//    if (container_.size() == 0) return std::nullopt;
//    return container_.back();
//  }

  const T &front() const
  {
    SpinLockGuard guard(lock_);
    return container_.front();
  }

  const T &back() const
  {
    SpinLockGuard guard(lock_);
    return container_.back();
  }

  T &front()
  {
    SpinLockGuard guard(lock_);
    return container_.front();
  }

  T &back()
  {
    SpinLockGuard guard(lock_);
    return container_.back();
  }

  void
  pop_front()
  {
    SpinLockGuard guard(lock_);
    container_.pop_front();
  }

  void
  pop_back()
  {
    SpinLockGuard guard(lock_);
    container_.pop_back();
  }

  void
  push_front(const T &item)
  {
    SpinLockGuard guard(lock_);
    container_.push_front(item);
  }

  void
  push_front(T &&item)
  {
    SpinLockGuard guard(lock_);
    container_.push_front(std::forward<T>(item));
  }

  template<typename... _Args> void
  emplace_front(_Args&&... __args)
  {
    SpinLockGuard guard(lock_);
    container_.emplace_front(std::forward<_Args>(__args)...);
  }

  void
  push_back(const T &item)
  {
    SpinLockGuard guard(lock_);
    container_.push_back(item);
  }

  void
  push_back(T &&item)
  {
    SpinLockGuard guard(lock_);
    container_.push_back(std::move<T>(item));
  }

  template<typename... _Args> void
  emplace_back(_Args&&... __args)
  {
    SpinLockGuard guard(lock_);
    container_.emplace_back(std::forward<_Args>(__args)...);
  }

  T extract_back()
  {
    SpinLockGuard guard(lock_);
    T value = container_.back();
    container_.pop_back();
    return value;
  }

  T extract_front()
  {
    SpinLockGuard guard(lock_);
    T value = container_.front();
    container_.pop_front();
    return value;
  }

  void
  for_each(std::function<void(T &)> locked_func)
  {
    SpinLockGuard guard(lock_);
    for (auto &item : container_)
      locked_func(item);
  }

  void
  for_each(std::function<void(const T &)> locked_func) const
  {
    SpinLockGuard guard(lock_);
    for (const auto &item : container_)
      locked_func(item);
  }

  void
  swap(LockedDeque<T, ALLOC> &rhs)
  {
    lock_both  (lock_, rhs.lock_);
    container_.swap(rhs.container_);
    unlock_both(lock_, rhs.lock_);
  }

  void
  swap(std::deque<T, ALLOC> &container)
  {
    SpinLockGuard guard(lock_);
    container_.swap(container);
  }

  size_t size() const
  {
    SpinLockGuard guard(lock_);
    return container_.size();
  }

  bool empty() const
  {
    SpinLockGuard guard(lock_);
    return container_.empty();
  }

  void clear()
  {
    SpinLockGuard guard(lock_);
    container_.clear();
  }

protected:
  container_type container_;

private:
  mutable SpinLock lock_;
};

