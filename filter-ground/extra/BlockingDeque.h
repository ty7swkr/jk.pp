/*
 * BlockingDeque.h
 *
 *  Created on: 2024. 8. 19.
 *      Author: tys
 */

#pragma once

#include <extra/MSignal.h>
#include <deque>
#include <atomic>

template<typename T>
class BlockingDeque
{
public:
  BlockingDeque  (bool open = true);
  ~BlockingDeque ();

  void  open      ();
  void  close     ();
  bool  is_open   () const;

  // @return -1: 닫혀있음, 0: 데이터
  int   push_front(const T              &item);
  int   push_back (const T              &item);

  int   push_back (std::deque<T>        &item);
  int   push_back (const std::deque<T>  &item);

  int   push_front(T                   &&item);
  int   push_back (T                   &&item);

  /**
   * 가변인자 템플릿을 이용한 push_back
   * 생성자에 필요한 변수만 받아서 큐 내부에서 T객체를 생성함.
   * 따라서 외부에서 생성시에 임시 객체가 생기지 않아 좀 더 나음.
   * @param args
   * @return
   */
  template<typename... Args> int emplace_back (Args&&... args);
  template<typename... Args> int emplace_front(Args&&... args);

  // @return -1: 닫혀있음 / 0: 데이터 / ETIMEDOUT : timeout
  int   pop_back          (T              &item,  const uint32_t &msec = 0);
  int   pop_front         (T              &item,  const uint32_t &msec = 0);
  int   pop               (std::deque<T>  &items, const uint32_t &msec = 0);
  int   pop_no_clear_items(std::deque<T>  &items, const uint32_t &msec = 0);

  // just wait
  int   wait              (const uint32_t &msec = 0);
  void  swap              (std::deque<T>  &item);

  std::deque<T>
  container() const;

  size_t
  size() const;

protected:
  bool          open_ = true;
  MSignal       signal_;
  std::deque<T> container_;
};

template<typename T>
BlockingDeque<T>::BlockingDeque(bool open)
: open_(open)
{
}

template<typename T>
BlockingDeque<T>::~BlockingDeque()
{
}

template<typename T> void
BlockingDeque<T>::close()
{
  auto lock = signal_.scoped_acquire_lock();
  open_ = false;
  signal_.notify_one(lock);
}

template<typename T> void
BlockingDeque<T>::open()
{
  auto lock = signal_.scoped_acquire_lock();
  open_ = true;
  signal_.notify_one(lock);
}

/**
 * @return 열려 있으면 true, 닫혀 있으면 false
 */
template<typename T> bool
BlockingDeque<T>::is_open() const
{
  auto lock = signal_.scoped_acquire_lock();
  return open_;
}

/**
 * @param item T 타입의 객체
 * @return -1: 닫혀있음, 0: 데이터
 */
template<typename T> int
BlockingDeque<T>::push_back(const T &item)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    container_.push_back(item);
    return 0;
  });
  return 0;
}

/**
 * @param item T 타입의 객체
 * @return -1: 닫혀있음, 0: 데이터
 */
template<typename T> int
BlockingDeque<T>::push_back(T &&item)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    container_.push_back(std::move(item));
    return 0;
  });
}

template<typename T> int
BlockingDeque<T>::push_back(std::deque<T> &item)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    if (container_.size() == 0)
      container_.swap(item);
    else
      container_.insert(container_.end(), item.begin(), item.end());

    return 0;
  });
}

template<typename T> int
BlockingDeque<T>::push_back(const std::deque<T> &item)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    if (container_.size() == 0)
      container_.swap(item);
    else
      container_.insert(container_.end(), item.begin(), item.end());

    return 0;
  });
}

template<typename T> template<typename... Args> int
BlockingDeque<T>::emplace_back(Args&&... args)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    container_.emplace_back(std::forward<Args>(args)...);

    return 0;
  });
}

/**
 * @param item T 타입의 객체
 * @return -1: 닫혀있음, 0: 데이터
 */
template<typename T> int
BlockingDeque<T>::push_front(const T &item)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    container_.push_front(item);

    return 0;
  });
}

/**
 * @param item T 타입의 객체
 * @return -1: 닫혀있음, 0: 데이터
 */
template<typename T> int
BlockingDeque<T>::push_front(T &&item)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    container_.emplace_front(std::move(item));

    return 0;
  });
}

/**
 * @param item T 타입의 객체
 * @return -1: 닫혀있음, 0: 데이터
 */
template<typename T> template<typename... Args> int
BlockingDeque<T>::emplace_front(Args&&... args)
{
  return signal_.notify_one([&]()
  {
    if (open_ == false)
      return -1;

    container_.emplace_front(std::forward<Args>(args)...);

    return 0;
  });
}

/**
 * @param items 받을 데이터
 * @param sec 기다릴 초
 * @param msec 기다릴 밀리초
 * @return -1: 닫혀있음 / 0: 데이터 / ETIMEDOUT : timeout
 */
template<typename T> int
BlockingDeque<T>::pop(std::deque<T> &items, const uint32_t &msec)
{
  items.clear();
  return pop_no_clear_items(items, msec);
}

/**
 * @param items 받을 데이터
 * @param sec 기다릴 초
 * @param msec 기다릴 밀리초
 * @return -1: 닫혀있음 / 0: 데이터 / ETIMEDOUT : timeout
 */
template<typename T> int
BlockingDeque<T>::pop_no_clear_items(std::deque<T> &items, const uint32_t &msec)
{
  while (true)
  {
    auto lock = signal_.scoped_acquire_lock();
    if (container_.size() > 0)
    {
      container_.swap(items);
      return 0;
    }

    if (open_ == false)
      return -1;

    if (signal_.wait(lock, msec) == false)
      return ETIMEDOUT;
  }
}

template<typename T> int
BlockingDeque<T>::wait(const uint32_t &msec)
{
  while (true)
  {
    auto lock = signal_.scoped_acquire_lock();
    if (container_.size() > 0)
    {
      container_.clear();
      return 0;
    }

    if (open_ == false)
      return -1;

    if (signal_.wait(lock, msec) == false)
      return ETIMEDOUT;
  }
}

/**
 * @param item 받을 데이터
 * @param sec 기다릴 초
 * @param msec 기다릴 밀리초
 * @return -1: 닫혀있음 / 0: 데이터 / ETIMEDOUT : timeout
 */
template<typename T> int
BlockingDeque<T>::pop_back(T &item, const uint32_t &msec)
{
  while (true)
  {
    auto lock = signal_.scoped_acquire_lock();
    if (container_.size() > 0)
    {
      item = std::move(container_.back());
      container_.pop_back();
      return 0;
    }

    if (open_ == false)
      return -1;

    if (signal_.wait(lock, msec) == false)
      return ETIMEDOUT;
  }
}

/**
 * @param item 받을 데이터
 * @param sec 기다릴 초
 * @param msec 기다릴 밀리초
 * @return -1: 닫혀있음 / 0: 데이터 / ETIMEDOUT : timeout
 */
template<typename T> int
BlockingDeque<T>::pop_front(T &item, const uint32_t &msec)
{
  while (true)
  {
    auto lock = signal_.scoped_acquire_lock();
    if (container_.size() > 0)
    {
      item = std::move(container_.front());
      container_.pop_front();
      return 0;
    }

    if (open_ == false)
      return -1;

    if (signal_.wait(lock, msec) == false)
      return ETIMEDOUT;
  }
}

template<typename T> void
BlockingDeque<T>::swap(std::deque<T> &items)
{
  auto lock = signal_.scoped_acquire_lock();
  container_.swap(items);
}

/**
 * @return 큐에 들어가 있는 데이터의 개수
 */
template<typename T> size_t
BlockingDeque<T>::size() const
{
  auto lock = signal_.scoped_acquire_lock();
  return container_.size();
}

/**
 * @return 복사된 deque
 */
template<typename T> std::deque<T>
BlockingDeque<T>::container() const
{
  auto lock = signal_.scoped_acquire_lock();
  return container_;
}

