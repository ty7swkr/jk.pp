/*
 * AtomicSptr.h
 *
 *  Created on: 2024. 11. 21.
 *      Author: tys
 */

#pragma once

#include <atomic>
#include <memory>

/**
//  C++11에서 shared_ptr을 사용하기 위한 클래스
//  C++11에서 shared_ptr이 std::atomic<T>에서 사용 못함 (C++20은 가능)
//  C++11에서는 전역함수 atomic_store, atomic_load에서만 가능.
//
//  C++11 std::atomic_store와 std::atomic_load (shared_ptr 특수화 스펙)
//  std::atomic_store
//
//  아래는 C++11 표준화 문서에 정의된 atomic_store, atomic_load 스펙입니다.
//  Prototype:
//  template<typename T> void atomic_store(std::shared_ptr<T>* obj, std::shared_ptr<T> desired);
//  Description:
//  원자적으로 std::shared_ptr<T> 값을 저장하며, 기존 값의 참조 카운트를 감소시키고 새로운 값의 참조 카운트를 증가시킴.
//  Header: <memory>
//  Namespace: std
//  std::atomic_store_explicit
//
//  Prototype:
//  template<typename T> void atomic_store_explicit(std::shared_ptr<T>* obj, std::shared_ptr<T> desired, std::memory_order order);
//  Description:
//  메모리 순서(std::memory_order)를 명시적으로 설정하여 std::shared_ptr<T> 값을 원자적으로 저장.
//  Header: <memory>
//  Namespace: std
//  std::atomic_load
//
//  Prototype:
//  template<typename T> std::shared_ptr<T> atomic_load(const std::shared_ptr<T>* obj);
//  Description:
//  원자적으로 std::shared_ptr<T> 값을 로드하며, 반환된 값의 참조 카운트를 증가시킴.
//  Header: <memory>
//  Namespace: std
//  std::atomic_load_explicit
//
//  Prototype:
//  template<typename T> std::shared_ptr<T> atomic_load_explicit(const std::shared_ptr<T>* obj, std::memory_order order);
//  Description:
//  메모리 순서(std::memory_order)를 명시적으로 설정하여 std::shared_ptr<T> 값을 원자적으로 로드.
//  메모리 순서 (std::memory_order)
//  Applicable Functions:
//  atomic_store_explicit, atomic_load_explicit
//  Options:
//  std::memory_order_relaxed
//  std::memory_order_consume
//  std::memory_order_acquire
//  std::memory_order_release
//  std::memory_order_acq_rel
//  std::memory_order_seq_cst
이 스펙은 C++11부터 지원되며, 
std::shared_ptr의 참조 카운트를 원자적으로 관리하여 멀티스레드 환경에서 안전하게 사용할 수 있습니다.
 */

template<typename T>
class AtomicSptr
{
public:
  AtomicSptr() = default;

  explicit AtomicSptr(std::shared_ptr<T> p)
  : sptr(std::move(p)) {}

  ~AtomicSptr() = default;

  void store(std::shared_ptr<T> new_value)
  {
    std::atomic_store(&sptr, std::move(new_value));
  }

  std::shared_ptr<T> load() const
  {
    return std::atomic_load(&sptr);
  }

  AtomicSptr& operator=(std::shared_ptr<T> new_value)
  {
    store(std::move(new_value));
    return *this;
  }

  // 복사 생성자
  AtomicSptr(const AtomicSptr &other)
  {
    store(other.load()); // 원자적으로 복사
  }

  // 복사 할당 연산자
  AtomicSptr &operator=(const AtomicSptr& other)
  {
    if (this != &other)
      store(other.load()); // 원자적으로 복사

    return *this;
  }

  // 이동 생성자
  AtomicSptr(AtomicSptr &&other) noexcept
  {
    store(std::move(other.load())); // 원자적으로 이동
  }

  // 이동 할당 연산자
  AtomicSptr& operator=(AtomicSptr &&other) noexcept
  {
    if (this != &other)
      store(std::move(other.load())); // 원자적으로 이동

    return *this;
  }

private:
  mutable std::shared_ptr<T> sptr; // mutable로 load에서 const 가능
};


