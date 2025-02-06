/*
 * BlockingDequeThread.h
 *
 *  Created on: 2024. 8. 19.
 *      Author: tys
 */

#pragma once

#include <extra/MThread.h>
#include <extra/BlockingDeque.h>

template<typename T = int>
class BlockingDequeThread : public MThread
{
public:
  BlockingDequeThread();
  virtual ~BlockingDequeThread() {}

          bool start() override;
  virtual bool stop ();


protected:
  BlockingDeque<T> waiter_;
};

template<typename T>
BlockingDequeThread<T>::BlockingDequeThread()
: waiter_(false)
{
}

template<typename T> bool
BlockingDequeThread<T>::start()
{
  if (waiter_.is_open() == true)
    return true;

  waiter_.open();
  return MThread::start();
}

template<typename T> bool
BlockingDequeThread<T>::stop()
{
  if (waiter_.is_open() == false)
    return true;

  waiter_.close();
  return MThread::join();
}
