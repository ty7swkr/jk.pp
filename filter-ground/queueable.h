/*
 * queue_item.h
 *
 *  Created on: 2024. 12. 2.
 *      Author: tys
 */

#pragma once

#include <memory>
#include <string>

/*
락프리 큐에서 사용하기 위한 shared_ptr 래퍼 클래스
boost 락프리큐는 소멸자가 없는 정적인 데이터만 가능.
블럭사이즈가 1만 사이즈인 경우 복사비용이 크므로
블럭을 shared_ptr로 래핑하여 1블럭 사이즈를 16바이트로 고정함.

메모리 특성:
- 정적 크기의 메모리를 alignas로 정렬하여 shared_ptr을 저장
- placement new를 사용해 해당 공간에 직접 객체 생성
- 명시적 소멸자 호출을 피하고 메모리 재사용을 가능하게 함

주의사항:
- shared_ptr의 레퍼런스 카운팅 관리를 위해 destroy() 호출 필요
- 소멸자를 명시적으로 선언하지 않음 (락프리 큐에서 메모리 재사용을 위해 허락하지않음(컴파일안됨))
- 복사/이동 생성자와 대입 연산자는 구현하지 않음

사용:
- 기본 생성자: 빈 shared_ptr 생성
- value 생성자: 값을 가진 shared_ptr 생성
- get(): 저장된 shared_ptr 참조 반환
- move(): shared_ptr을 반환하고 현재 객체 파괴
- destroy(): shared_ptr 리셋 (레퍼런스 카운트 감소)
*/
template<typename T>
struct queueable_t
{
  static constexpr size_t sptr_size = sizeof(std::shared_ptr<T>);
  alignas(std::shared_ptr<T>) char item[sptr_size] = { 0x00, };

  queueable_t()
  {
    new (item) std::shared_ptr<T>();  // placement new로 초기화
  }

  explicit queueable_t(const T &value)
  {
    new (item) std::shared_ptr<T>(std::make_shared<T>(value));
  }

  explicit queueable_t(std::shared_ptr<T> &value)
  {
    new (item) std::shared_ptr<T>(value);
  }

  explicit queueable_t(std::shared_ptr<T> &&value)
  {
    new (item) std::shared_ptr<T>(std::move(value));
  }

  std::shared_ptr<T> &get()
  {
    return *reinterpret_cast<std::shared_ptr<T>*>(item);
  }

  std::shared_ptr<T> take()
  {
    auto res = *reinterpret_cast<std::shared_ptr<T>*>(item);
    this->destroy();
    return res;
  }

  std::shared_ptr<T> move()
  {
    return this->take();
  }

  void destroy()
  {
    this->get().reset();
  }
};

using queueable_str_t = queueable_t<std::string>;

