#include <type_traits>
#include <utility>
#include <stdexcept>
#include <string>
#include <initializer_list>

#pragma once

struct nullopt_t
{
  static const nullopt_t &get()
  {
    static const nullopt_t instance;
    return instance;
  }
private:
  nullopt_t() = default;
};

#define nullopt nullopt_t::get()

template<typename T>
class Optional
{
public:
  // constructors
  Optional() noexcept : has_value_(false) {}
  Optional(nullopt_t) noexcept : has_value_(false) {}  // nullopt 생성자

  // C-style 문자열을 받는 생성자 추가
  template<typename U = T>
  Optional(const char *str,
           typename std::enable_if<std::is_same<U, std::string>::value>::type* = nullptr)
  {
    storage_ = std::string(str);
    has_value_ = true;
  }

  // initializer list를 받는 생성자 추가
  // initializer list를 받는 생성자
  template<typename U = T, typename E = typename U::value_type>  // 컨테이너의 요소 타입
  Optional(const std::initializer_list<E> &init)
  {
    storage_ = U(init);  // 컨테이너 생성
    has_value_ = true;
  }

  template<typename U = T, typename E = typename U::value_type>
  Optional(std::initializer_list<const char*> init)
  {
    storage_.reserve(init.size());  // 성능 최적화를 위한 예약
    for (const auto& item : init)
      storage_.emplace_back(item);  // const char*를 string으로 변환

    has_value_ = true;
  }

  Optional(const Optional &other)
  {
    storage_    = other.storage_;
    has_value_  = other.has_value_;
  }

  Optional(Optional &&other) noexcept
  {
    storage_    = std::forward<T>(other.storage_);
    has_value_  = other.has_value_;
  }

  Optional(const T &value)
  {
    storage_   = value;
    has_value_ = true;
  }

  Optional(T &&value)
  {
    storage_ = std::forward<T>(value);
    has_value_ = true;
  }

  // destructor
  ~Optional()
  {
    clear();
  }

  // assignment operators
  Optional& operator=(nullopt_t) noexcept // nullopt 대입 연산자 추가
  {
    clear();
    return *this;
  }

  Optional& operator=(const Optional &other)
  {
    storage_   = other.storage_;
    has_value_ = other.has_value_;
    return *this;
  }

  Optional& operator=(Optional &&other) noexcept
  {
    if (this == &other)
      return *this;

    storage_   = std::forward<T>(other.storage_);
    has_value_ = other.has_value_;
    return *this;
  }

  // value access
  bool has_value() const noexcept
  {
    return has_value_;
  }

  explicit operator bool() const noexcept
  {
    return has_value_;
  }

//  bool operator==(const bool &value) const
//  {
//    return has_value_ == value;
//  }

  T &value() &
  {
    //    if (!has_value_)
    //      throw std::runtime_error("Bad optional access");

    return storage_;
  }

  const T& value() const &
  {
    //    if (has_value_ == false)
    //      throw std::runtime_error("Bad optional access");
    return storage_;
  }

  T &&value() &&
  {
    //    if (!has_value_)
    //      throw std::runtime_error("Bad optional access");
    return std::move(storage_);
  }

  const T &&value() const &&
  {
    //    if (!has_value_)
    //      throw std::runtime_error("Bad optional access");
    return std::move(storage_);
  }

  T value_or(const T &default_value) const
  {
    return has_value_ == true ? value() : default_value;
  }

  T value_or(T&& default_value) const
  {
    return has_value_ == true ? value() : std::move(default_value);
  }

  // emplace
  template<typename... Args>
  T &emplace(Args &&... args)
  {
    clear();
    storage_ = T(std::forward<Args>(args)...);
    has_value_ = true;
    return storage_;
  }

  // reset
  void reset() noexcept
  {
    clear();
  }

  // pointer-like operators
  T* operator->()
  {
    return &storage_;
  }

  const T* operator->() const
  {
    return &storage_;
  }

  T &operator*() &
  {
    return storage_;
  }

  const T &operator*() const &
  {
    return storage_;
  }

  // comparison operators with nullopt
  bool operator==(nullopt_t) const noexcept
  {
    return has_value_ == false;
  }

  bool operator!=(nullopt_t) const noexcept
  {
    return has_value_ == true;
  }

private:
  // aligned storage_ for T
  T     storage_;
  bool  has_value_ = false;

  void clear()
  {
    has_value_ = false;
  }
};

// nullopt에 대한 전역 비교 연산자
template<typename T>
bool operator==(nullopt_t, const Optional<T> &opt) noexcept
{
  return opt.has_value() == false;
}

template<typename T>
bool operator!=(nullopt_t, const Optional<T> &opt) noexcept
{
  return opt.has_value() != false;
}

