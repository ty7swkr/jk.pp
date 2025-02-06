/*
 * TlInstance.h
 *
 *  Created on: 2024. 12. 23.
 *      Author: tys
 */

#pragma once

#include <map>
#include <memory>

/**
 * @brief Thread Local Storage 기반의 인스턴스 관리 템플릿 클래스
 * @details 각 쓰레드별로 독립적인 인스턴스를 관리하기 위한 클래스입니다.
 *          쓰레드마다 별도의 저장소를 가지며, 인스턴스의 주소값을 키로 사용합니다.
 *
 * @tparam T 관리할 객체의 타입
 */
template <typename T>
class TlInstance
{
public:
  /**
   * @brief 기본 생성자
   */
  TlInstance() {}

  /**
   * @brief self_는 이동시에도 여전히 이전 메모리 주소를 가지고 있게되므로
   * 여기서는 이동돼지 못하도록 삭제합니다.
   */
  TlInstance(TlInstance&&) = delete;
  TlInstance& operator=(TlInstance&&) = delete;

  /**
   * @brief 초기값을 가지는 생성자
   * @param value 초기화할 값
   */
  TlInstance(const std::shared_ptr<T> &value)
  {
    set(self_, value);
  }

  /**
   * @brief 소멸자
   * @details 현재 인스턴스에 해당하는 저장소 항목을 제거합니다.
   */
  virtual ~TlInstance()
  {
    // singleton인 경우 소멸시점이 애매함. 그래서 뻑남. 그래서 삭제.
    // 삭제하려면 명시적으로 clear사용할 것.
    // 하긴 그래서 shared_ptr로 하긴 했지.
    // storage_.erase(self_);
  }

  /**
   * @brief 현재 인스턴스의 값을 설정
   * @param value 설정할 값
   * @return 현재 인스턴스에 대한 참조
   */
  virtual TlInstance<T> &
  set(const std::shared_ptr<T> &value)
  {
    storage_[self_] = value;
    return *this;
  }

  /**
   * @brief 현재 인스턴스의 값이 저장소에 존재하는지 확인
   * @return 값이 존재하면 true, 그렇지 않으면 false
   */
  virtual bool
  has()
  {
    return storage_.count(self_) > 0;
  }

  /**
   * @brief 현재 인스턴스의 값을 가져옴
   * @return 저장된 값의 shared_ptr
   */
  virtual std::shared_ptr<T>
  get()
  {
    return storage_[self_];
  }

  /**
   * @brief 현재 인스턴스의 값을 상수 형태로 가져옴
   * @return 저장된 값의 const shared_ptr, 값이 없으면 nullptr
   */
  virtual const std::shared_ptr<T>
  get() const
  {
    auto it = storage_.find(self_);
    if (it == storage_.end()) return nullptr;
    return it->second;
  }

  void
  clear()
  {
    storage_.clear();
  }

protected:
  /** @brief 현재 인스턴스의 주소값을 정수형으로 변환하여 저장 (키로 사용) */
  uintptr_t self_ = reinterpret_cast<uintptr_t>(this);

  /** @brief 쓰레드별 저장소. 인스턴스 주소를 키로 사용하는 맵 */
  static thread_local std::map<uintptr_t, std::shared_ptr<T>> storage_;
};

/** @brief 쓰레드별 저장소의 정적 멤버 선언 */
template <typename T>
thread_local std::map<uintptr_t, std::shared_ptr<T>> TlInstance<T>::storage_;
