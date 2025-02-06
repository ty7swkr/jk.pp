/*
  *Expected.h
 *
  * Created on: 2024. 8. 16.
  *     Author: tys
 */

#pragma once

#include <utility>
#include <type_traits>
#include <exception>
#include <string>

/*
 * Expected 사용 예제:
 *
 1. 기본적인 성공/실패 케이스
Expected<int, std::string> divide(int a, int b)
{
  if (b == 0)
    return make_unexpected("Division by zero");

  return a / b;
}

void example_basic()
{
  auto result = divide(10, 2);
  if (result == true)
    std::cout << "Result: " << result.value() << std::endl;  // 출력: Result: 5

  auto error_case = divide(10, 0);
  if (error_case == false)
    std::cout << "Error: " << error_case.error() << std::endl;  // 출력: Error: Division by zero
}

// 2. pair를 사용한 에러 코드와 메시지
Expected<int, std::pair<int, std::string>> parse_number(const std::string& str)
{
  try
  {
    return std::stoi(str);
  }
  catch(...)
  {
    return make_unexpected(std::make_pair(1, "Invalid number format"));
  }
}

void example_pair_error()
{
  auto result = parse_number("abc");
  if (result == false)
  {
    auto error = result.error();
    std::cout << "Error code: " << error.first
              << ", Message: " << error.second << std::endl;
  }
}

// 3. 사용자 정의 타입
struct UserData
{
  std::string name;
  int age;
};

Expected<UserData, std::string> get_user(int id)
{
  if (id < 0)
    return make_unexpected("Invalid user ID");

  return UserData{"John", 30};
}

void example_custom_type()
{
  auto user = get_user(-1);
  if (user == false)
    std::cout << "Failed to get user: " << user.error() << std::endl;
}

// 4. 람다와 함께 사용
void example_lambda()
{
  std::function<Expected<int, std::pair<int, std::string>>(bool)> process =
      [](bool value) -> Expected<int, std::pair<int, std::string>>
      {
        if (value == false)
        {
          return make_unexpected(std::make_pair(404, "Process failed"));
        }
        return 42;
      };

  auto result = process(false);
  if (result == false)
  {
    std::cout << "Error " << result.error().first
              << ": " << result.error().second << std::endl;
  }
}
 *
 */

template<typename E>
class Unexpected
{
  E error_;

public:
  Unexpected(const E &e) : error_(e) {}
  Unexpected(E&&e) : error_(std::move(e)) {}

  const E  &error() const & { return error_; }
        E  &error()  &      { return error_; }
        E &&error() &&      { return std::move(error_); }
};

template<typename E>
Unexpected<typename std::decay<E>::type> make_unexpected(E&&e)
{
  return Unexpected<typename std::decay<E>::type>(std::forward<E>(e));
}

// 문자열 리터럴 전용 오버로드
inline Unexpected<std::string>
make_unexpected(const char *e)
{
  return Unexpected<std::string>(std::string(e));
}

// 문자열 리터럴 pair 전용 오버로드
template<typename T>
Unexpected<std::pair<T, std::string>>
make_unexpected(const std::pair<T, const char *> &e)
{
  return Unexpected<std::pair<T, std::string>>(
      std::pair<T, std::string>(e.first, std::string(e.second))
  );
}

template<typename T>
Unexpected<std::pair<T, std::string>>
make_unexpected(std::pair<T, const char *> &&e)
{
  return Unexpected<std::pair<T, std::string>>(
      std::make_pair(std::move(e.first), std::string(e.second))
  );
}

template<typename T, typename E>
class Expected
{
public:
  // 기본 생성자 삭제
  Expected() = delete;

  // 복사 생성자
  Expected(const Expected &other) : has_value_(other.has_value_)
  {
    if (has_value_)
      new (&val_) T(other.val_);
    else
      new (&unexpected_) E(other.unexpected_);
  }

  // 이동 생성자
  Expected(Expected &&other) : has_value_(other.has_value_)
  {
    if (has_value_)
      new (&val_) T(std::move(other.val_));
    else
      new (&unexpected_) E(std::move(other.unexpected_));
  }

  // 복사 대입 연산자
  Expected &operator=(const Expected &other)
  {
    if (this != &other)
    {
      destroy();
      has_value_ = other.has_value_;
      if (has_value_)
        new (&val_) T(other.val_);
      else
        new (&unexpected_) E(other.unexpected_);
    }
    return *this;
  }

  // 이동 대입 연산자
  Expected &operator=(Expected &&other)
  {
    if (this != &other)
    {
      destroy();
      has_value_ = other.has_value_;
      if (has_value_)
        new (&val_) T(std::move(other.val_));
      else
        new (&unexpected_) E(std::move(other.unexpected_));
    }
    return *this;
  }

  // 값 생성자들
  Expected(const T &v) : has_value_(true)
  {
    new (&val_) T(v);
  }

  Expected(T &&v) : has_value_(true)
  {
    new (&val_) T(std::move(v));
  }

  Expected(const Unexpected<E> &u) : has_value_(false)
  {
    new (&unexpected_) E(u.error());
  }

  Expected(Unexpected<E> &&u) : has_value_(false)
  {
    new (&unexpected_) E(std::move(u.error()));
  }

  // 상태 확인
  explicit operator bool()    const { return has_value_; }
  bool has_value ()           const { return has_value_; }
  bool operator==(bool value) const { return has_value_ == value; }
  bool operator!=(bool value) const { return has_value_ != value; }

  // 값 접근자
  T &value() &
  {
    return val_;
  }

  const T &value() const &
  {
    return val_;
  }

  T &&value() &&
  {
    return std::move(val_);
  }

  // 에러 접근자
  E &error() &
  {
    return unexpected_;
  }

  const E &error() const &
  {
    return unexpected_;
  }

  E &&error() &&
  {
    return std::move(unexpected_);
  }

//  // 값 접근자
//  T &value() &
//  {
//    if (!has_value_) throw std::bad_exception();
//    return val_;
//  }
//
//  const T &value() const &
//  {
//    if (!has_value_) throw std::bad_exception();
//    return val_;
//  }
//
//  T &&value() &&
//  {
//    if (!has_value_) throw std::bad_exception();
//    return std::move(val_);
//  }
//
//  // 에러 접근자
//  E &error() &
//  {
//    if (has_value_) throw std::bad_exception();
//    return unexpected_;
//  }
//
//  const E &error() const &
//  {
//    if (has_value_) throw std::bad_exception();
//    return unexpected_;
//  }
//
//  E &&error() &&
//  {
//    if (has_value_) throw std::bad_exception();
//    return std::move(unexpected_);
//  }

  // 포인터 연산자
        T *operator->()         { return &val_; }
  const T *operator->() const   { return &val_; }
        T &operator *() &       { return val_; }
  const T &operator *() const & { return val_; }
        T&&operator *() &&      { return std::move(val_); }

  ~Expected()
  {
    destroy();
  }

private:
  void destroy()
  {
    if (has_value_)
      val_.~T();
    else
      unexpected_.~E();
  }

  bool has_value_;
  union
  {
    T val_;
    E unexpected_;
  };
};


