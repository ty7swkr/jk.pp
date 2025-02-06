/*
 * ThreadUniqueIndexer.h
 *
 *  Created on: 2025. 1. 20.
 *      Author: tys
 */

#pragma once

#include <atomic>

#define thread_uindex ThreadUniqueIndexer<>::index()

/**
 * @brief Thread 별로 고유한 index를 생성하는 클래스.
 */
template<int = 1>
class ThreadUniqueIndexer
{
public:
  static int64_t index()
  {
    if (index_ < 0) { index_ = ++alloc_; }
    return index_;
  }

protected:
  static thread_local int64_t index_;
  static std::atomic<int64_t> alloc_;
};

// C++11에서 static inline을 위한 꼼수. = template 클래스로 만들기.
template<int N> thread_local int64_t ThreadUniqueIndexer<N>::index_ = -1;
template<int N> std::atomic<int64_t> ThreadUniqueIndexer<N>::alloc_(0);
