/*
 * main.cpp
 *
 *  Created on: 2024. 12. 24.
 *      Author: tys
 */
#pragma once

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <memory>
#include <map>
#include <vector>
#include <sstream>
#include <functional>
#include <type_traits>

//          +------------------+
//          |    MJsonValue    |
//          +------------------+
//                   |
//         +---------------------+
//         v                     v
//  +--------------+      +---------------+
//  |  MJsonArray  |      |  MJsonObject  |
//  +--------------+      +---------------+

class MJsonObject;
class MJsonArray;
class MJsonValue;

/**

Managed Json Object

# 개요

rapidjson을 사용하면 throw가 없기 때문에 아래와 같이 작성 하게 되지만,
if (doc.HasMember("users") &&
    doc["users"].IsArray() &&
    doc["users"].Size() > 0 &&
    doc["users"][0].HasMember("contact") &&
    doc["users"][0]["contact"].HasMember("email") &&
    doc["users"][0]["contact"]["email"].IsString())
{
  std::string email = doc["users"][0]["contact"]["email"].GetString();
  return true;
}
else
{
  return false;
}

MJsonObject를 사용하면
try
{
  std::string email = doc["users"][0]["contact"]["email"].as_str_or("no-email");
  return true;
}
catch (const std::runtime_error &e)
{
  std::cout << e.what() << std::endl;
  return false;
}

# 접근 방식
  - 체이닝을 이용한 접근
  - 계층적 접근[함수형 프로그래밍] : required, required_for, optional, optional_or
  - 언제 throw 되는가

## 체이닝을 이용한 접근
  MJsonObject doc = parse(json);

  // 필드가 json에 없는 경우 예외를 발생합니다.
  std::string name  = doc["users"][0]["name"  ].as_string();
  int         age   = doc["users"][0]["age"   ].as_int();
  double      score = doc["users"][0]["points"].as_double();

  // 마지막 필드가 json에 없는 경우 예외를 발생하지 않고 인자값을 리턴합니다.
  // 단, 그 앞 [user or 0]이 없는 경우는 예외를 발생시킵니다.
  std::string name  = doc["users"][0]["name"  ].as_string_or("홍길똥");
  int         age   = doc["users"][0]["age"   ].as_int_or(30);
  double      score = doc["users"][0]["points"].as_double_or(0);


## 계층적 접근 : required, required_for
// JSON 구조:
  std::string json = R"(
  {
     "player":
     {
         "name": "전사1",
         "level": 10,
         "items":
         [
             {"name": "롱소드", "damage": 50},
             {"name": "방패",  "damage": 30}
         ],
         "stats":
         {
             "hp": 100,
             "mp": 50
         }
     }
  })";

  MJsonObject doc = parse(json);

  // player 접근
  doc.required("required", [](const MJsonObject& player)
  {
    std::string name = player["name"].as_string();  // "전사1"
    int level        = player["level"].as_int();    // 10

    // items 배열 접근
    player.required_for([]("items", const MJsonArray &items, size_t index)
    {
      std::string item_name = items[index]["name"  ].as_string();
      int damage            = items[index]["damage"].as_int();
      int durability        = items[index]["durability"].as_int_or(100);  // durability가 없으면 100을 기본값으로
    });

    // stats 객체 접근
    // 체이닝 없이 JSON 구조의 원하는 계층에 접근합니다. 해당 계층이 없는 경우 throw를 발생시킵니다.
    player.required([]("stats", const MJsonObject& stats)
    {
      int hp = stats["hp"].as_int();   // 100
      int mp = stats["mp"].as_int();   // 50
    });

    // skills 객체가 있으면 처리, 없으면 기본값 사용
    player.optional("skills", [](const MJsonObject& skills)
    {
      // skills 객체가 있는 경우의 처리
    },
    []() { // 생략할 수도 있습니다.
      // skills 객체가 없을 때의 기본 처리
      std::cout << "기본 스킬만 사용 가능" << std::endl;
    });
  });

# 언제 throw 되는가 - 지연 평가(lazy evaluation)
   { "data": [1, 2, 3] }

   MJsonObject obj = parse(json);
   obj["show"];               // 유효하지 않은 객체이나 아무 액션이 없으므로 이때는 throw하지 않습니다.
   obj["show"].as_int();      // 유효하지 않은 객체이므로 as_int호출시 throw합니다.
   obj["show"].as_int_or(0);  // 유효하지 않은 객체이나 as_int_or호출 하였으므로 throw하지 않고 기본값인 0을 반환합니다.
   auto &doc = obj["show"];   // 이때도 참조 전달이므로 throw하지 않습니다.(복사나 move시는 throw)

 */

/**
 * @brief JSON 파일을 파싱하는 최상위 함수
 * @param filename JSON 파일 이름
 * @param func JSON 파일 읽기가 성공한 후 파일 내용을 전달받을 함수
 * @return 파싱된 MJsonObject 객체
 * @throw std::runtime_error 파일 열기 실패, JSON 파싱 실패 시
 */
MJsonObject parse_file(const std::string &filename, std::function<void(const std::string &json_conf)> func = nullptr);

/**
 * @brief JSON 문자열을 파싱하는 최상위 함수
 * @param json 파싱할 JSON 문자열
 * @return 파싱된 MJsonObject 객체
 * @throw std::runtime_error JSON 파싱 실패 시
 * @details rapidjson을 사용하여 JSON 문자열을 파싱하고,
 *          내부 객체 구조로 변환합니다. 이 함수는 JSON 파싱의
 *          시작점으로 사용됩니다.
 */
MJsonObject parse(const std::string &json);

/**
 * @class MJsonValue
 * @brief JSON 값을 나타내는 기본 클래스
 * @details 모든 JSON 값의 기본이 되는 클래스입니다.
 *          값의 타입(배열, 객체, 기본값)과 데이터 타입(null, boolean, 숫자, 문자열)을 관리하며,
 *          값에 대한 타입 검사와 변환 기능을 제공합니다.
 *
 *          주요 기능:
 *          - 값의 구조적 타입 확인 (배열, 객체, 기본값)
 *          - 값의 데이터 타입 확인 (null, boolean, 숫자, 문자열)
 *          - 다양한 데이터 타입으로의 변환
 *          - 객체의 멤버나 배열의 요소에 대한 접근
 *          - 조건부 실행을 위한 required/optional 메서드 제공
 */
class MJsonValue
{
public:
  /**
   * @brief 기본 생성자
   */
  MJsonValue() {}
  MJsonValue(std::shared_ptr<MJsonValue> val);
  virtual ~MJsonValue() {}

  /**
   * @brief 값의 구조적 타입을 확인하는 함수들
   * @return 해당 타입이면 true, 아니면 false
   * @details 값이 배열, 객체, 기본값 중 어떤 타입인지 확인합니다.
   */
  bool  is_array  () const { exception(); return type_ == MJsonValue::ARRAY; }
  bool  is_object () const { exception(); return type_ == MJsonValue::OBJECT;}
  bool  is_value  () const { exception(); return type_ == MJsonValue::VALUE; }

  /**
   * @brief 값의 데이터 타입을 확인하는 함수들
   * @return 해당 타입이면 true, 아니면 false
   * @details 값이 null, boolean, 숫자(int32/int64/double), 문자열 중 어떤 타입인지 확인합니다.
   */
  bool  is_null   () const { exception(); return value_type_ == MJsonValue::NIL;    }
  bool  is_bool   () const { exception(); return value_type_ == MJsonValue::BOOL;   }
  bool  is_int32  () const { exception(); return value_type_ == MJsonValue::NUMBER; }
  bool  is_int64  () const { exception(); return value_type_ == MJsonValue::NUMBER; }
  bool  is_double () const { exception(); return value_type_ == MJsonValue::NUMBER; }
  bool  is_string () const { exception(); return value_type_ == MJsonValue::STRING; }

  /**
   * @brief 값을 특정 타입으로 변환하는 함수들
   * @return 변환된 값
   * @throw std::runtime_error 값이 해당 타입이 아닌 경우
   * @details 값을 boolean, 숫자(double/float/int/int64/uint32/uint64), 문자열로 변환합니다.
   */
  bool        as_bool         () const { exception(); throw_value_type(BOOL  ); return value_ == "true"; }
  std::string as_bool_str     () const { exception(); throw_value_type(BOOL  ); return value_; }
  double      as_double       () const { exception(); throw_value_type(NUMBER); return atof (value_.c_str()); }
  float       as_float        () const { exception(); throw_value_type(NUMBER); return atof (value_.c_str()); }
  int         as_int          () const { exception(); throw_value_type(NUMBER); return atoi (value_.c_str()); }
  int64_t     as_int64        () const { exception(); throw_value_type(NUMBER); return atoll(value_.c_str()); }
  uint32_t    as_uint32       () const { exception(); throw_value_type(NUMBER); return static_cast<uint32_t>(atoll(value_.c_str())); }
  uint64_t    as_uint64       () const { exception(); throw_value_type(NUMBER); return strtoull(value_.c_str(), nullptr, 10); }
  const std::string &as_string() const { exception(); throw_value_type(STRING); return value_; }
  const std::string &as_str   () const { exception(); throw_value_type(STRING); return value_; }

  /**
   * @brief 값을 특정 타입으로 변환하거나 기본값을 반환하는 함수들
   * @param value 기본값
   * @return 변환된 값 또는 기본값
   * @throw std::runtime_error 값이 존재하지만 해당 타입이 아닌 경우만, 값이 없는경우는 기본값을 반환합니다.
   * @details 값이 존재하지 않으면 기본값을 반환하고, 존재하면 해당 타입으로 변환합니다.
   */
  bool        as_bool_or      (bool     value) const { if (exception_) return value; return value_ == "true"; }
  double      as_double_or    (double   value) const { if (exception_) return value; return atof (value_.c_str()); }
  float       as_float_or     (float    value) const { if (exception_) return value; return atof (value_.c_str()); }
  int         as_int_or       (int      value) const { if (exception_) return value; return atoi (value_.c_str()); }
  int64_t     as_int64_or     (int64_t  value) const { if (exception_) return value; return atoll (value_.c_str());}
  uint32_t    as_uint32_or    (uint32_t value) const { if (exception_) return value; return static_cast<uint64_t>(atoll(value_.c_str())); }
  uint64_t    as_uint64_or    (uint64_t value) const { if (exception_) return value; return static_cast<uint64_t>(atoll(value_.c_str())); }
  const std::string &as_str_or(const std::string &value) const
  {  if (exception_) return value; return value_; }

  /**
   * @brief 객체의 멤버나 배열의 요소에 접근하는 연산자
   * @param name 객체의 멤버 이름
   * @param index 배열의 인덱스
   * @return 멤버나 요소에 대한 참조
   * @throw std::runtime_error 멤버나 요소가 존재하지 않는 경우
   */
  /// 기본적으로 required 입니다!
  const MJsonValue  &operator[](const std::string &name) const;
  const MJsonValue  &operator[](size_t index) const;

  /**
   * @brief 객체나 배열에 대한 작업을 수행하는 함수
   * @param handler 작업을 수행할 함수
   * @return 작업이 수행된 객체나 배열에 대한 참조
   * @throw std::runtime_error 객체나 배열이 존재하지 않는 경우
   * @details 객체나 배열에 대해 주어진 handler 함수를 실행하고 자신을 반환합니다.
   */
  const MJsonValue &required(const std::string &name, std::function<void(const MJsonValue &)> handler) const
  { return required<MJsonValue>(name, handler); }

  /**
   * @brief 배열의 각 요소에 대해 작업을 수행하는 함수
   * @param index 배열의 인덱스
   * @param handler 각 요소를 매개변수로 받는 작업 함수
   */
  const MJsonValue &required(size_t index, std::function<void(const MJsonValue &)> handler) const
  { return required<MJsonValue>(index, handler); }

  /**
   * @brief 배열의 각 요소와 인덱스에 대해 작업을 수행하는 함수
   * @param name 배열의 이름
   * @param handler 각 요소와 인덱스를 매개변수로 받는 작업 함수
   * @throw std::runtime_error 배열이 존재하지 않는 경우
   * @return 작업이 수행된 배열에 대한 참조
   */
  const MJsonArray &required_for(const std::string &name, std::function<void(const MJsonArray &, size_t)> handler) const { return required_for<MJsonValue> (name, handler); }

  /**
   * @brief 배열의 각 요소와 인덱스에 대해 작업을 수행하는 함수
   * @param index 배열의 인덱스
   * @param handler 각 요소와 인덱스를 매개변수로 받는 작업 함수
   * @throw std::runtime_error 배열이 존재하지 않는 경우
   * @return 작업이 수행된 배열에 대한 참조
   * @details 배열의 각 요소와 인덱스에 대해 주어진 handler 함수를 실행하고 자신을 반환합니다.
   *          handler 함수는 index에 해당하는 요소와 인덱스를 매개변수로 받습니다.
   */
  const MJsonArray &required_for(size_t index, std::function<void(const MJsonArray &, size_t)> handler) const { return required_for<MJsonValue> (index, handler); }

  /**
   * @brief 이 객체가 존재할 때는 handler를, 없을 때는 alternative를 실행하는 함수
   * @param name json오브젝트 이름
   * @param handler 객체나 배열이 존재할 때 실행할 함수
   * @param alternative 객체나 배열이 존재하지 않을 때 실행할 함수
   * @throw noexcept
   */
  void optional(const std::string &name,
                std::function<void(const MJsonValue &)> handler,
                std::function<void()> alternative = nullptr) const { optional<MJsonValue>(name, handler, alternative); }

  void optional(size_t index,
                std::function<void(const MJsonValue &)> handler,
                std::function<void()> alternative = nullptr) const { optional<MJsonValue>(index, handler, alternative); }

  /**
   * @brief 이 객체가 존재할 때는 각 요소에 대해 handler를, 없을 때는 alternative를 실행하는 함수
   * @param name json오브젝트 이름
   * @param handler 배열의 각 요소와 인덱스를 매개변수로 받는 작업 함수
   * @param alternative 배열이 존재하지 않을 때 실행할 함수
   * @throw noexcept
   */
  void optional_for(const std::string &name,
                    std::function<void(const MJsonArray &, size_t)> handler,
                    std::function<void()> alternative = nullptr) const;

  void optional_for(size_t index,
                    std::function<void(const MJsonArray &, size_t)> handler,
                    std::function<void()> alternative = nullptr) const;

  /**
   * @brief 이 객체를 다른 타입으로 변환하는 연산자
   * @return 변환된 객체
   * @throw std::runtime_error 값이 존재하지 않는 경우
   *        이 객체의 type이 MJsonArray(as_array), MJsonObject(as_object)가 아닌 경우
   * @details 이 객체를 MJsonArray나 MJsonObject로 변환하여 반환합니다.
   */
  operator const MJsonObject &() const { return as_object(); }
  operator const MJsonArray  &() const { return as_array (); }

  /**
   * @brief 값의 내부 문자열을 반환하는 함수
   * @return 내부 문자열에 대한 참조
   */
  const std::string &inner_value() const { return value_; }

  /**
   * @brief 내부 객체를 std::shared_ptr<MJsonValue>으로 이동하는 함수.
   * @return T로 변환된 객체
   * @throw std::runtime_error 값이 존재하지 않는 경우, json객체와 타입이 일치하지 않는 경우
   * @details 값이 존재하면 T로 변환된 객체를 반환하고, 이후에는 이 객체는 빈 객체가 됩니다.
   */
  template<typename T> T
  take() const { return T(take_ptr()); }

  /**
   * @brief 이 객체를 MJsonArray(as_array()) or MJsonObject(as_object)로 변환하는 함수
   * @return 변환된 MJsonArray 객체
   * @throw std::runtime_error 값이 존재하지 않는 경우,
   *        이 객체의 type이 MJsonArray(as_array), MJsonObject(as_object)가 아닌 경우
   */
  const MJsonArray  &as_array () const;
  const MJsonObject &as_object() const;

public:
  /**
   * @brief 객체나 배열에 대한 접근 여부를 확인하는 함수들
   * @param name 객체의 멤버 이름
   * @param index 배열의 인덱스
   * @return 해당 멤버나 요소가 존재하면 true, 아니면 false
   */
  virtual bool    has(const std::string &name) const;
  virtual bool    has(size_t index           ) const;

  /// 배열의 크기를 반환하는 함수
  virtual size_t  size() const;

protected:
  virtual std::shared_ptr<MJsonValue> by        (const std::string &name) const { (void)name;  return nullptr;}
  virtual std::shared_ptr<MJsonValue> at        (size_t index)            const { (void)index; return nullptr;}
  virtual bool                        has_impl  (const std::string &name) const { (void)name;  return false;  }
  virtual bool                        has_impl  (size_t index)            const { (void)index; return false;  }
  virtual std::shared_ptr<MJsonValue> take_impl () { return std::make_shared<MJsonValue>(std::move(*this)); }
  std::shared_ptr<MJsonValue> take_ptr() const;

protected:
  template<typename T>
  const T &required(std::function<void(const T &)> handler) const;

  template<typename T> const T &
  required(const std::string &name, std::function<void(const T &)> handler) const;

  template<typename T> const T &
  required(size_t index, std::function<void(const T &)> handler) const;

  template<typename T>
  const MJsonArray &required_for(const std::string &name, std::function<void(const T &, size_t)> handler) const;

  template<typename T>
  const MJsonArray &required_for(size_t index, std::function<void(const T &, size_t)> handler) const;

protected:
  template<typename T>
  void optional(const std::string &name, std::function<void(const T &)> handler, std::function<void()> alternative) const;

  template<typename T>
  void optional(size_t index, std::function<void(const T &)> handler, std::function<void()> alternative) const;

//  template<typename T>
//  void optional(std::function<void(const T &)> handler, std::function<void()> alternative) const;

protected:
  enum TYPE      { VALUE, ARRAY, OBJECT }; // JSON 값의 기본 타입을 나타내는 열거형
  enum ValueType { NIL = 0, BOOL = 1, NUMBER, STRING }; // rapidjson에서 지원하는 값 타입들을 나타내는 열거형

  MJsonValue(const TYPE &type) : type_(type) {}

protected:
  bool exception_ = false;
  void exception() const { if (exception_) throw std::runtime_error(path_ + name_ + ": not found"); }

  template<typename T> const T &get_self_const() const;
  template<typename T>       T &get_self()       { return const_cast<T&>(get_self_const<T>());}
  template<typename T> const T &get_self() const { return                get_self_const<T>(); }

  void throw_value_type(const ValueType &value_type) const
  {
    if (is_value() == false      ) throw std::runtime_error(path_ + name_ + ": expected MJsonValue, got " + type_str());
    if (value_type != value_type_) throw std::runtime_error(path_ + name_ + ": expected " + value_type_str(value_type) + ", got " + value_type_str(value_type_));
    return;
  }

protected:
  std::string path_;
  std::string name_;
  std::string value_;
  ValueType   value_type_ = NIL;
  TYPE        type_       = VALUE;

  // 임시보관장소임.
  mutable std::shared_ptr<const MJsonValue> entity_;

  std::string type_str() const
  {
    switch(type_)
    {
      case VALUE:     return "MJsonValue";
      case OBJECT:    return "MJsonObject";
      case ARRAY:     return "MJsonArray";
      default: return "unknown";
    }
  }

  std::string value_type_str(ValueType value_type) const
  {
    switch(value_type)
    {
      case NIL:     return "null";
      case BOOL:    return "boolean";
      case NUMBER:  return "number";
      case STRING:  return "string";
      default: return "unknown";
    }
  }

  friend MJsonValue  parse_value (std::string path, const std::string &name, const rapidjson::Value &value);
  friend MJsonArray  parse_array (std::string path, const std::string &name, const rapidjson::Value &value);
  friend MJsonObject parse_object(std::string path, const std::string &name, const rapidjson::Value &value);
  friend class MJsonArray;
  friend class MJsonObject;
};

/**
 * @class MJsonArray
 * @brief JSON 배열을 나타내는 클래스
 * @details 순차적인 JSON 요소들의 컬렉션을 관리합니다.
 *          인덱스 기반 접근과 요소 추가 기능을 제공합니다.
 */
class MJsonArray : public MJsonValue
{
public:
  MJsonArray() : MJsonValue(MJsonValue::ARRAY) {}
  MJsonArray(const MJsonArray &val) : MJsonValue(MJsonValue::ARRAY) {  this->operator=(val); }
  MJsonArray(      MJsonArray &&val);
  MJsonArray(std::shared_ptr<MJsonValue> &&val);

  MJsonArray  &operator=(const MJsonArray& rhs);
  size_t      size      () const override { exception(); return elements.size(); }

  std::vector<std::shared_ptr<MJsonValue>> elements;

protected:
  template<typename T> void   add       (const T &element);
  std::shared_ptr<MJsonValue> at        (size_t index) const override;
  virtual bool                has_impl  (size_t index) const override { return index < elements.size(); }
  std::shared_ptr<MJsonValue> take_impl () override { return std::make_shared<MJsonArray>(std::move(*this)); }

protected:
  friend MJsonValue  parse_value (std::string path, const std::string &name, const rapidjson::Value &value);
  friend MJsonArray  parse_array (std::string path, const std::string &name, const rapidjson::Value &value);
  friend MJsonObject parse_object(std::string path, const std::string &name, const rapidjson::Value &value);
  friend class MJsonValue;
};

/**
 * @class MJsonObject
 * @brief JSON 객체를 나타내는 클래스
 * @details 키-값 쌍으로 이루어진 JSON 요소들의 컬렉션을 관리합니다.
 *          문자열 키를 통한 멤버 접근과 추가 기능을 제공합니다.
 */
class MJsonObject : public MJsonValue
{
public:
  MJsonObject() : MJsonValue(MJsonValue::OBJECT) {}
  MJsonObject(const MJsonObject &val) : MJsonValue(MJsonValue::OBJECT) { this->operator =(val); }
  MJsonObject(      MJsonObject &&val);
  MJsonObject(std::shared_ptr<MJsonValue> &&val);

  MJsonObject &operator=(const MJsonObject &rhs);
  size_t      size      () const override { return members.size(); }
  MJsonObject take();

  std::map<std::string, std::shared_ptr<MJsonValue>> members;
protected:
  template<typename T> void   add       (const std::string &name, const T &element);
  std::shared_ptr<MJsonValue> by        (const std::string &name) const override;
  virtual bool                has_impl  (const std::string &name) const override { return members.count(name) > 0; }
  std::shared_ptr<MJsonValue> take_impl () override { return std::make_shared<MJsonObject>(std::move(*this)); }

protected:
  friend MJsonValue  parse_value (std::string path, const std::string &name, const rapidjson::Value &value);
  friend MJsonArray  parse_array (std::string path, const std::string &name, const rapidjson::Value &value);
  friend MJsonObject parse_object(std::string path, const std::string &name, const rapidjson::Value &value);
  friend class MJsonValue;
};

#include <extra/MJsonObject.inl>
#include <extra/MJsonValue.inl>
#include <extra/MJsonArray.inl>

//////////////////////////////////////////////////////////////////////////////
// Parse 함수 선언부
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief JSON 값을 파싱하는 함수
 * @param path 현재 파싱 중인 값의 경로
 * @param name 현재 파싱 중인 값의 이름
 * @param value rapidjson으로 파싱된 값
 * @return 파싱된 MJsonValue 객체
 * @details rapidjson의 값을 내부 MJsonValue 타입으로 변환합니다.
 *          null, boolean, 숫자, 문자열 타입을 처리합니다.
 */
MJsonValue   parse_value (std::string path, const std::string &name, const rapidjson::Value &value);

/**
 * @brief JSON 배열을 파싱하는 함수
 * @param path 현재 파싱 중인 배열의 경로
 * @param name 현재 파싱 중인 배열의 이름
 * @param value rapidjson으로 파싱된 배열
 * @return 파싱된 MJsonArray 객체
 * @details rapidjson의 배열을 내부 MJsonArray 타입으로 변환합니다.
 *          배열의 각 요소를 재귀적으로 파싱합니다.
 */
MJsonArray   parse_array (std::string path, const std::string &name, const rapidjson::Value &value);

/**
 * @brief JSON 객체를 파싱하는 함수
 * @param path 현재 파싱 중인 객체의 경로
 * @param name 현재 파싱 중인 객체의 이름
 * @param value rapidjson으로 파싱된 객체
 * @return 파싱된 MJsonObject 객체
 * @details rapidjson의 객체를 내부 MJsonObject 타입으로 변환합니다.
 *          객체의 각 멤버를 재귀적으로 파싱합니다.
 */
MJsonObject  parse_object(std::string path, const std::string &name, const rapidjson::Value &value);

/**
 * @brief JSON 값을 파싱하는 함수
 * @param name 현재 파싱 중인 값의 경로명
 * @param value rapidjson으로 파싱된 값
 * @return 파싱된 MJsonValue 객체
 * @details rapidjson의 값을 내부 MJsonValue 타입으로 변환합니다.
 *          null, boolean, 숫자, 문자열 타입을 처리합니다.
 */
inline MJsonValue
parse_value(std::string path, const std::string &name, const rapidjson::Value &value)
{
  MJsonValue result;
  result.path_ = path;
  result.name_ = name;
  switch (value.GetType())
  {
    case rapidjson::kNullType:
    {
      result.value_type_  = MJsonValue::NIL;
      result.value_.clear();
      break;
    }
    case rapidjson::kTrueType:
    case rapidjson::kFalseType:
    {
      result.value_type_  = MJsonValue::BOOL;
      result.value_       = value.GetBool() ? "true" : "false";
      break;
    }
    case rapidjson::kNumberType:
    {
      result.value_type_  = MJsonValue::NUMBER;
      if      (value.IsInt())   { result.value_ = std::to_string(value.GetInt());   }
      else if (value.IsInt64()) { result.value_ = std::to_string(value.GetInt64()); }
      else if (value.IsUint())  { result.value_ = std::to_string(value.GetUint());  }
      else if (value.IsUint64()){ result.value_ = std::to_string(value.GetUint64());}
      else if (value.IsDouble() || value.IsFloat())
      {
        std::ostringstream oss;
        oss.precision(17); // IEEE double의 전체 정밀도
        oss << value.GetDouble();
        result.value_ = oss.str();
      }
      break;
    }
    case rapidjson::kStringType:
    {
      result.value_type_ = MJsonValue::STRING;
      result.value_ = value.GetString();
      break;
    }
    default:
    {
      throw std::runtime_error("Unsupported JSON value type for name: " + path + name);
    }
  }

  return result;
}

/**
 * @brief JSON 배열을 파싱하는 함수
 * @param name 현재 파싱 중인 배열의 경로명
 * @param value rapidjson으로 파싱된 배열
 * @return 파싱된 MJsonArray 객체
 * @details rapidjson의 배열을 내부 MJsonArray 타입으로 변환합니다.
 *          배열의 각 요소를 재귀적으로 파싱합니다.
 */
inline MJsonArray
parse_array(std::string path, const std::string &name, const rapidjson::Value &value)
{
  MJsonArray array;
  array.path_ = path;
  array.name_ = name;

  for (rapidjson::SizeType index = 0; index < value.Size(); ++index)
  {
    const auto &element      = value[index];
    std::string element_name = "[" + std::to_string(index) + "]";
    path = path + name + "/";
    switch (element.GetType())
    {
      case rapidjson::kArrayType:
      {
        array.add(parse_array(path, element_name, element));
        break;
      }
      case rapidjson::kObjectType:
      {
        array.add(parse_object(path, element_name, element));
        break;
      }
      case rapidjson::kNullType:
      case rapidjson::kFalseType:
      case rapidjson::kTrueType:
      case rapidjson::kNumberType:
      case rapidjson::kStringType:
      {
        array.add(parse_value(path, element_name, element));
        break;
      }
      default:
      {
        throw std::runtime_error("Unsupported array element type at index " + path);
      }
    }
  }

  return array;
}

/**
 * @brief JSON 객체를 파싱하는 함수
 * @param name 현재 파싱 중인 객체의 경로명
 * @param value rapidjson으로 파싱된 객체
 * @return 파싱된 MJsonObject 객체
 * @details rapidjson의 객체를 내부 MJsonObject 타입으로 변환합니다.
 *          객체의 각 멤버를 재귀적으로 파싱합니다.
 */
inline MJsonObject
parse_object(std::string path, const std::string &name, const rapidjson::Value &value)
{
  MJsonObject object;
  object.path_ = path;
  object.name_ = name;

  path += name + "/";

  for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
  {
    const std::string &sub_name     = it->name.GetString();
    const auto        &sub_element  = it->value;

    switch (sub_element.GetType())
    {
      case rapidjson::kArrayType:
      {
        object.add(sub_name, parse_array(path, sub_name, sub_element));
        break;
      }
      case rapidjson::kObjectType:
      {
        object.add(sub_name, parse_object(path, sub_name, sub_element));
        break;
      }
      case rapidjson::kNullType:
      case rapidjson::kFalseType:
      case rapidjson::kTrueType:
      case rapidjson::kNumberType:
      case rapidjson::kStringType:
      {
        object.add(sub_name, parse_value(path, sub_name, sub_element));
        break;
      }
      default:
      {
        throw std::runtime_error("Unsupported object member type for name: " + path + name);
      }
    }
  }

  return object;
}

/**
 * @brief JSON 객체를 파싱하는 오버로드된 함수
 * @param value rapidjson으로 파싱된 객체
 * @return 파싱된 MJsonObject 객체
 * @details 루트 객체를 파싱할 때 사용되는 간단한 오버로드 버전입니다.
 *          빈 경로와 이름으로 parse_object를 호출합니다.
 */
inline MJsonObject
parse_object(const rapidjson::Value &value)
{
  return parse_object("", "", value);
}

inline const char *
get_rapidjson_error_msg(rapidjson::ParseErrorCode code) {
  switch (code)
  {
    case rapidjson::kParseErrorNone:                          return "No error";
    case rapidjson::kParseErrorDocumentEmpty:                 return "Empty document";
    case rapidjson::kParseErrorDocumentRootNotSingular:       return "Document root must not follow by other values";
    case rapidjson::kParseErrorValueInvalid:                  return "Invalid value";
    case rapidjson::kParseErrorObjectMissName:                return "Missing object member name";
    case rapidjson::kParseErrorObjectMissColon:               return "Missing colon after object member name";
    case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket: return "Missing comma or } after object member";
    case rapidjson::kParseErrorArrayMissCommaOrSquareBracket: return "Missing comma or ] after array member";
    case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex: return "Invalid unicode hex in string";
    case rapidjson::kParseErrorStringUnicodeSurrogateInvalid: return "Invalid unicode surrogate in string";
    case rapidjson::kParseErrorStringEscapeInvalid:           return "Invalid escape character in string";
    case rapidjson::kParseErrorStringMissQuotationMark:       return "Missing quotation mark in string";
    case rapidjson::kParseErrorStringInvalidEncoding:         return "Invalid string encoding";
    case rapidjson::kParseErrorNumberTooBig:                  return "Number is too big";
    case rapidjson::kParseErrorNumberMissFraction:            return "Missing fraction part in number";
    case rapidjson::kParseErrorNumberMissExponent:            return "Missing exponent in number";
    default:                                                  return "Unknown error";
  }
}
/**
 * @brief JSON 문자열을 파싱하는 최상위 함수
 * @param json 파싱할 JSON 문자열
 * @return 파싱된 MJsonObject 객체
 * @throw std::runtime_error JSON 파싱 실패 시, 에러 메시지와 파싱 위치 정보를 포함한 예외 발생
 * @details rapidjson을 사용하여 JSON 문자열을 파싱하고,
 *          내부 객체 구조로 변환합니다. 이 함수는 JSON 파싱의
 *          시작점으로 사용됩니다.
 */
inline MJsonObject
parse(const std::string &json)
{
  rapidjson::Document doc;
  rapidjson::ParseResult result = doc.Parse(json.c_str());

  if (result.IsError())
  {
    std::stringstream ss;
    ss << "JSON parse error at offset " << result.Offset() << ": "
       << get_rapidjson_error_msg(result.Code());

    // 에러 발생 위치의 컨텍스트 추가
    size_t start = result.Offset() > 30 ? result.Offset() - 30 : 0;
    size_t length = 60;
    if (start + length > json.length())
      length = json.length() - start;

    ss << "\nContext: " << json.substr(start, length);

    throw std::runtime_error(ss.str());
  }

  // Start parsing from root
  return parse_object(doc);
}

#include <fstream>     // 파일 입출력 (ifstream)

/**
 * @brief 파일에서 JSON 문자열을 읽어 파싱하는 함수
 * @param filename JSON 파일 이름
 * @param func JSON 문자열을 읽은 후 호출할 함수
 * @return 파싱된 MJsonObject 객체
 * @throw std::runtime_error 파일 열기 실패 시
 * @details 파일에서 JSON 문자열을 읽어 파싱하고, 내부 객체 구조로 변환합니다.
 */
inline MJsonObject
parse_file(const std::string &filename,
           std::function<void(const std::string &json_conf)> func)
{
  std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
  if (file.is_open() == false)
    throw std::runtime_error("Failed to open file: " + filename);

  // Read file content into string
  std::stringstream buffer;
  buffer << file.rdbuf();
  if (func != nullptr) func(buffer.str());
  return parse(buffer.str());
}
