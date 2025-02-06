/**
 * @file  LockedObject.h
 * @brief 변수를 원자성을 가진 변수로 만드는 클래스
*/
#pragma once

#include <extra/SpinLockGuard.h>
#include <functional>
#include <utility>

/**
 * @brief 변수를 원자성을 가진 변수로 만드는 클래스
 * std::atomic은 trivially copyable type만 지원해서 만듬
 * @see Atomic.h
*/

template<typename T>
class LockedObject
{
public:
  LockedObject() {}

  /// @brief 복사 생성자
  LockedObject(LockedObject<T> &&rhs);
  LockedObject(const LockedObject<T> &rhs);

  // @brief 값이 오는 경우 컴파일러의 암묵적 형변환 금지함. explicit
  LockedObject(T       &&rhs);
  LockedObject(const T  &rhs);

  /// @brief 원자성을 가진 대입 = 연산자
  LockedObject<T> &operator=(const LockedObject<T>  &rhs);
  LockedObject<T> &operator=(LockedObject<T>       &&rhs);
  LockedObject<T> &operator=(const T                &rhs);
  LockedObject<T> &operator=(T                     &&rhs);

  /// @brief 원자성을 가진 비교 연산자
  bool operator==(const T &rhs) const;
  bool operator==(const LockedObject<T> &rhs) const;

  /// @brief 원자성을 가진 비교 연산자
  bool operator!=(const T &rhs) const;
  bool operator!=(const LockedObject<T> &rhs) const;

  T operator->() const { return load(); }

//  bool execute(std::function<bool(T &value)> locked_func)
//  {
//    SpinLockGuard guard(lock_);
//    return locked_func(value_);
//  }
//
//  bool execute(std::function<bool(const T &value)> locked_func) const
//  {
//    SpinLockGuard guard(lock_);
//    return locked_func(value_);
//  }

  template<typename R>
  R    execute(std::function<R(T &value)> locked_func)
  {
    SpinLockGuard guard(lock_);
    return locked_func(value_);
  }

  template<typename R>
  R    execute(std::function<R(const T &value)> locked_func) const
  {
    SpinLockGuard guard(lock_);
    return locked_func(value_);
  }

  void execute(std::function<void(T &value)> locked_func)
  {
    SpinLockGuard guard(lock_);
    locked_func(value_);
  }

  void execute(std::function<void(const T &value)> locked_func) const
  {
    SpinLockGuard guard(lock_);
    locked_func(value_);
  }

  template<typename R>
  R    execute_lockfree(std::function<R(const T &value)> lockfree_func) const
  {
    T value;
    {
      SpinLockGuard guard(lock_);
      T value = value_;
    }
    return lockfree_func(value_);
  }

  template<typename R>
  R    execute_lockfree(std::function<R(T &value)> lockfree_func)
  {
    T value;
    {
      SpinLockGuard guard(lock_);
      T value = value_;
    }
    return lockfree_func(value_);
  }

  void store(T &value)
  {
    SpinLockGuard guard(lock_);
    value_ = value;
  }

  void store(const T &value)
  {
    SpinLockGuard guard(lock_);
    value_ = value;
  }

  T   operator()() const { return load(); }
  T   load      () const;

  // unlocked reference
  T       &ref  ()       { return value_; }
  const T &ref  () const { return value_; }

private:
  T                value_;
  mutable SpinLock lock_;
};

template<typename T>
LockedObject<T>::LockedObject(LockedObject<T> &&rhs)
{
  T value = rhs.load();
  SpinLockGuard guard(lock_);
  value_ = std::move(value);
}

template<typename T>
LockedObject<T>::LockedObject(const LockedObject<T> &rhs)
{
  T value = rhs.load();
  SpinLockGuard guard(lock_);
  value_ = std::move(value);
}

template<typename T>
LockedObject<T>::LockedObject(T &&rhs)
{
  SpinLockGuard guard(lock_);
  value_ = std::forward<T>(rhs);
}

template<typename T>
LockedObject<T>::LockedObject(const T &rhs)
{
  SpinLockGuard guard(lock_);
  value_ = rhs;
}

template<typename T> LockedObject<T> &
LockedObject<T>::operator=(const LockedObject<T> &rhs)
{
  // 안전한 복사를 위해 이중락을 피함.
  T value = rhs.load();
  SpinLockGuard guard(lock_);
  value_ = value;

  return *this;
}

template<typename T> LockedObject<T> &
LockedObject<T>::operator=(LockedObject<T> &&rhs)
{
  // 안전한 복사를 위해 이중락을 피함.
  T value = rhs.load();
  SpinLockGuard guard(lock_);
  value_ = std::move(value);

  return *this;
}

template<typename T> LockedObject<T> &
LockedObject<T>::operator=(const T &rhs)
{
  SpinLockGuard guard(lock_);
  value_ = rhs;

  return *this;
}

template<typename T> LockedObject<T> &
LockedObject<T>::operator=(T &&rhs)
{
  SpinLockGuard guard(lock_);
  value_ = std::move(rhs);

  return *this;
}

/**
 * @param value 비교할 변수값
 * @return 같으면 true, 다르면 false
 */
template<typename T> bool
LockedObject<T>::operator==(const T &rhs) const
{
  SpinLockGuard guard(lock_);
  return value_ == rhs;
}

template<typename T> bool
LockedObject<T>::operator==(const LockedObject<T> &rhs) const
{
  T rhs_value = rhs.load();
  SpinLockGuard guard(lock_);
  return value_ == rhs_value;
}

template<typename T> bool
LockedObject<T>::operator!=(const T &rhs) const
{
  SpinLockGuard guard(lock_);
  return value_ != rhs;
}

template<typename T> bool
LockedObject<T>::operator!=(const LockedObject<T> &rhs) const
{
  T rhs_value = rhs.load();
  SpinLockGuard guard(lock_);
  return value_ != rhs_value;
}

template<typename T> T
LockedObject<T>::load() const
{
  SpinLockGuard guard(lock_);
  return value_;
}
