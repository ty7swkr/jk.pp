#pragma once
#include <mutex>

template<typename T>
class Singleton
{
public:
  virtual ~Singleton() = default;

  static T& ref()
  {
    static T instance;  // static 객체로 변경
    return instance;
  }

  // 복사 생성과 대입을 막아요
  Singleton(const Singleton &) = delete;
  Singleton &operator=(const Singleton &) = delete;

protected:
  Singleton() = default;
};
