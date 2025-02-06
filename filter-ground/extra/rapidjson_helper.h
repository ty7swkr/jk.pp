/*
 * rapid_helper.h
 *
 *  Created on: 2024. 11. 13.
 *      Author: tys
 */

#pragma once

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>   // PrettyWriter 포함
#include <extra/Optional.h>
#include <string>
#include <vector>
#include <functional>

/**
 * @brief rapidjson 라이브러리에서 자주 사용되는 타입들의 별칭 정의
 */
using rapid_gvalue= rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>; ///< Generic Value 타입
using rapid_al    = rapidjson::Document::AllocatorType;  ///< Allocator 타입
using rapid_value = rapidjson::Value;                    ///< JSON Value 타입
using rapid_doc   = rapidjson::Document;                 ///< JSON Document 타입

// rapidjson 라이브러리는 예외처리를 해주지 않으면 assert되므로 꼭 예외처리를 해주어야 한다.
// 여기서 char 단일 캐릭터는 그냥 스트링으로 처리한다. char name = 'a' -json-> "name": "a"

/**
 * @brief JSON 문서를 문자열로 변환하는 함수
 * 
 * @param doc 변환할 JSON 문서
 * @param pretty 결과를 보기 좋게 포맷팅할지 여부 (기본값: false)
 * @return std::string 변환된 JSON 문자열
 * 
 * @details
 * - pretty가 true인 경우 들여쓰기와 줄바꿈이 포함된 가독성 좋은 형태로 변환
 * - pretty가 false인 경우 공백 없이 한 줄로 변환
 */
inline std::string to_json(const rapidjson::Document &doc, bool pretty = false)
{
  // JSON 문자열로 변환
  rapidjson::StringBuffer buffer;

  if (pretty == true)
  {
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
  }

  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);
  return buffer.GetString();
}

/**
 * @brief JSON 값을 기본 타입으로 변환하는 연산자 오버로딩
 * 
 * @tparam R JSON 값 타입
 * @param lhs 변환될 대상 변수
 * @param rhs JSON 값
 * 
 * @details 문자열이 비어있는 경우 null 문자(0x00)를 할당하고, 그렇지 않으면 첫 번째 문자를 할당
 */
template<typename R> void operator<<(char &lhs, const R &rhs) { if (strlen(rhs.GetString()) == 0) lhs = 0x00; else lhs = rhs.GetString()[0]; }
template<typename R> void operator<<(int16_t      &lhs, const R &rhs) { lhs = rhs.GetInt();    }
template<typename R> void operator<<(int32_t      &lhs, const R &rhs) { lhs = rhs.GetInt();    }
template<typename R> void operator<<(int64_t      &lhs, const R &rhs) { lhs = rhs.GetInt64();  }
template<typename R> void operator<<(long long    &lhs, const R &rhs) { lhs = rhs.GetInt64();  }
template<typename R> void operator<<(float        &lhs, const R &rhs) { lhs = rhs.GetFloat();  }
template<typename R> void operator<<(double       &lhs, const R &rhs) { lhs = rhs.GetDouble(); }
template<typename R> void operator<<(std::string  &lhs, const R &rhs) { lhs = rhs.GetString(); }

/**
 * @brief JSON 값을 문자 배열로 변환하는 연산자 오버로딩
 * 
 * @tparam R JSON 값 타입
 * @tparam N 문자 배열의 크기
 * @param var 변환될 문자 배열
 * @param obj JSON 값
 * 
 * @details
 * - 배열을 0x00으로 초기화
 * - JSON 문자열을 배열 크기에 맞게 복사
 * - null 종료 문자를 보장하기 위해 N-1 크기까지만 복사
 */
template <typename R, size_t N>
void operator<<(char (&var)[N], const R &obj)
{
  // strncpy를 사용할때마다 컴파일러 경고가 있기 때문에 바꿈.
  memset(var, 0x00, N);

  const char *str = obj.GetString();
  if (str == nullptr)
    return;

  size_t str_len = 0;
  for (size_t index = 0; index < N && str[index] != '\0'; ++index)
    str_len = index+1;

  std::memcpy(var, obj.GetString(), std::min(str_len, N-1));
}

/**
 * @brief JSON 배열을 long long 배열로 변환하는 연산자 오버로딩
 * 
 * @tparam R JSON 값 타입
 * @tparam N 배열의 크기
 * @param var 변환될 long long 배열
 * @param obj JSON 배열
 * 
 * @details JSON 배열의 각 요소를 long long 타입으로 변환하여 배열에 저장
 */
template <typename R, size_t N>
void operator<<(long long (&var)[N], const R &obj)
{
  for (rapidjson::SizeType index = 0; index < N; ++index)
    var[index] = obj[index].GetInt64();
}

/// add_member ///////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief JSON 객체에서 특정 멤버가 유효한지 확인하는 함수.
 *
 * @tparam R JSON 객체의 타입.
 * @param var 확인할 변수 (변수 자체는 사용하지 않지만 변수 타입을 템플릿에서 결정하기 위해 사용, 더미파라미터).
 * @param obj 확인할 JSON 객체.
 * @param name 확인할 JSON 필드의 이름.
 * @return true 멤버가 존재하고 문자열 타입일 경우.
 * @return false 멤버가 존재하지 않거나 문자열 타입이 아닌 경우.
 *
 * @details
 * - JSON 객체 `obj`에 `name`이라는 키가 존재하는지 확인합니다.
 * - 해당 키의 값이 문자열 타입인지 확인합니다.
 */
template<typename R> bool is_valid(const char         &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsString()) return false; return true; }
template<typename R> bool is_valid(const int16_t      &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsInt()   ) return false; return true; }
template<typename R> bool is_valid(const int32_t      &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsInt()   ) return false; return true; }
template<typename R> bool is_valid(const int64_t      &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsInt64() ) return false; return true; }
template<typename R> bool is_valid(const long long    &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsInt64() ) return false; return true; }
template<typename R> bool is_valid(const std::string  &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsString()) return false; return true; }
template<typename R> bool is_valid(const double       &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsDouble()) return false; return true; }
template<typename R> bool is_valid(const float        &var, const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str())) return false; if (!obj[name.c_str()].IsFloat() ) return false; return true; }

template<typename R, size_t N>
bool is_valid(const char (&var)[N],     const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str()) || !obj[name.c_str()].IsString()) return false; return true; }

template<typename R, size_t N, size_t M>
bool is_valid(const char (&var)[N][M],  const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str()) || !obj[name.c_str()].IsArray()) return false; return true; }

template<typename R, size_t N>
bool is_valid(const long long (&var)[N],const R &obj, const std::string &name) { (void)var; if (!obj.HasMember(name.c_str()) || !obj[name.c_str()].IsArray()) return false; return true; }

/**
 * @brief JSON 객체에 value 값을 멤버로 추가하는 함수.
 *
 * @tparam R JSON 객체의 타입.
 * @param obj 멤버를 추가할 JSON 객체.
 * @param name 추가할 멤버의 이름.
 * @param value 추가할  값.
 * @param al 메모리 할당을 위한 `rapid_al(rapidjson::Document::AllocatorType)` 객체.
 *
 * @details
 * - `name`을 키로, `value`를 값으로 JSON 객체 `obj`에 새로운 멤버를 추가합니다.
 * - `char` 타입의 값은 `std::string`으로 변환하여 처리합니다.
 */
template<typename R> void add_member(R &obj, const std::string &name, const char        &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), rapid_value(std::string(1, value).c_str(), al), al); }
template<typename R> void add_member(R &obj, const std::string &name, const int16_t     &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), value, al); }
template<typename R> void add_member(R &obj, const std::string &name, const int32_t     &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), value, al); }
template<typename R> void add_member(R &obj, const std::string &name, const int64_t     &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), value, al); }
template<typename R> void add_member(R &obj, const std::string &name, const long long   &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), static_cast<int64_t>(value), al); }
template<typename R> void add_member(R &obj, const std::string &name, const float       &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), value, al); }
template<typename R> void add_member(R &obj, const std::string &name, const double      &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), value, al); }
template<typename R> void add_member(R &obj, const std::string &name, const std::string &value, rapid_al &al) { obj.AddMember(rapid_value(name.c_str(), al).Move(), rapid_value(value.c_str(), al), al); }

template <typename R, size_t N> void // 배열처리
add_member(R &obj, const std::string &name, const char (&value)[N], rapid_al &al)
{
  obj.AddMember(rapid_value(name.c_str(), al).Move(), rapid_value(std::string(value, N-1).c_str(), al), al);
}

template <typename R, size_t N> void // 배열처리
add_member(R &obj, const std::string &name, const long long (&value)[N], rapid_al &al)
{
  rapid_value array(rapidjson::kArrayType);
  array.Reserve(N, al);  // 용량을 예약

  for (size_t index = 0; index < N; ++index)
    array.PushBack(rapid_value().SetInt64(value[index]), al);

  obj.AddMember(rapid_value(name.c_str(), al), array, al);
}

template <typename R, size_t N, size_t M> void // 2차원 문자열(배열, char) 처리
add_member(R &obj, const std::string &name, const char (&value)[N][M], rapid_al &al)
{
  rapid_value array(rapidjson::kArrayType); // 최상위 배열

  // 문자열 배열 value를 순회하며 JSON 배열로 변환
  for (size_t index = 0; index < N; ++index)
    array.PushBack(rapid_value(std::string(value[index], M).c_str(), al).Move(), al);

  // obj에 name과 array 추가
  obj.AddMember(rapidjson::Value(name.c_str(), al).Move(), array, al);
}

// 벡터 배열 처리
/**
 * @brief 벡터를 JSON 배열 멤버로 추가하는 함수
 * 
 * @tparam RAPID_OBJECT JSON 객체 타입
 * @tparam T 벡터 요소의 타입
 * @param obj JSON 객체
 * @param name 추가할 멤버의 이름
 * @param values 추가할 벡터 값
 * @param al 메모리 할당자
 * @return RAPID_OBJECT& 수정된 JSON 객체에 대한 참조
 * 
 * @details 벡터의 각 요소를 JSON 배열로 변환하여 객체의 멤버로 추가
 */
template<typename RAPID_OBJECT, typename T> RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const std::vector<T> &values, rapidjson::Document::AllocatorType &al)
{
  rapid_value array(rapidjson::kArrayType);

  for (auto &value : values)
    array.PushBack(value, al);

  obj.AddMember(rapid_value(name.c_str(), al).Move(), array, al);
  return obj;
}

// 벡터 배열 처리
/**
 * @brief Optional 벡터를 JSON 배열 멤버로 추가하는 함수
 * 
 * @tparam RAPID_OBJECT JSON 객체 타입
 * @tparam T 벡터 요소의 타입
 * @param obj JSON 객체
 * @param name 추가할 멤버의 이름
 * @param values Optional 벡터 값
 * @param al 메모리 할당자
 * @return RAPID_OBJECT& 수정된 JSON 객체에 대한 참조
 * 
 * @details
 * - Optional 값이 nullopt인 경우 아무 작업도 수행하지 않음
 * - 값이 존재하는 경우 벡터를 JSON 배열로 변환하여 객체의 멤버로 추가
 */
template<typename RAPID_OBJECT, typename T> RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const Optional<std::vector<T>> &values, rapidjson::Document::AllocatorType &al)
{
  if (values == nullopt)
    return obj;

  return add_member(obj, name, values.value(), al);
}

// 특수화 스트링 벡터 배열 처리
template<typename RAPID_OBJECT> inline RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const std::vector<std::string> &values, rapidjson::Document::AllocatorType &al)
{
  rapid_value array(rapidjson::kArrayType);

  for (auto &value : values)
    array.PushBack(rapidjson::Value(value.c_str(), al).Move(), al);

  obj.AddMember(rapid_value(name.c_str(), al).Move(), array, al);
  return obj;
}

// 특수화 스트링 벡터 배열 처리
template<typename RAPID_OBJECT> inline RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const Optional<std::vector<std::string>> &values, rapidjson::Document::AllocatorType &al)
{
  if (values == nullopt)
    return obj;

  return add_member(obj, name, values.value(), al);
}

template<typename R> void add_member(R &obj, const std::string &name, const Optional<std::string> &value, rapid_al &al)
{
  if (value == nullopt) return;
  obj.AddMember(rapid_value(name.c_str(), al).Move(), rapid_value(value.value().c_str(), al), al);
}

template<typename R> void add_member(R &obj, const std::string &name, const Optional<int32_t> &value, rapid_al &al)
{
  if (value == nullopt) return;
  obj.AddMember(rapid_value(name.c_str(), al).Move(), value.value(), al);
}

template<typename R> void add_member(R &obj, const std::string &name, const Optional<int64_t> &value, rapid_al &al)
{
  if (value == nullopt) return;
  obj.AddMember(rapid_value(name.c_str(), al).Move(), value.value(), al);
}

/// set_value ///////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief JSON 필드 값을 단일 val 변수에 설정하는 함수.
 *
 * @tparam R JSON 객체의 타입.
 * @param val 값을 설정할 `char` 변수.
 * @param obj 데이터를 포함한 JSON 객체.
 * @param name JSON 필드 이름.
 * @throws std::pair<bool, std::string> 필드가 유효하지 않거나 존재하지 않을 경우 예외 발생.
 *
 * @details
 * - JSON 객체 `obj`에서 `name` 키의 값을 `char` 변수 `val`에 설정합니다.
 * - 필드가 존재하지 않거나 타입이 다른 경우 예외를 발생시킵니다.
 */
template<typename R> void set_value(char        &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R> void set_value(int16_t     &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R> void set_value(int32_t     &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R> void set_value(int64_t     &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R> void set_value(long long   &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R> void set_value(float       &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R> void set_value(double      &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R> void set_value(std::string &val, const R &obj, const std::string &name ) { if (is_valid(val, obj, name) == false) throw std::string("Not found or Invalid type ") + name; val << obj[name.c_str()];}
template<typename R, typename T> void
set_value(std::vector<T> &vals, const R &obj, const std::string &name)
{
  if (obj.HasMember(name.c_str()) == false)
    throw std::string("Not found or Invalid type " + name);

  if (obj[name.c_str()].IsArray() == false)
    throw std::string("Not found or Invalid type " + name);

  const auto &items = obj[name.c_str()];

  vals.resize(items.Size());
  for (rapidjson::SizeType index = 0; index < items.Size(); ++index)
    vals[index] << items[index];
}

/**
 * @brief JSON 필드 값을 단일 `char` 배열에 설정하는 함수.
 *
 * @tparam R JSON 객체의 타입.
 * @tparam N `char` 배열의 크기.
 * @param val 값을 설정할 `char` 배열 참조.
 * @param obj 데이터를 포함한 JSON 객체.
 * @param name JSON 필드 이름.
 * @throws std::pair<bool, std::string> 필드가 유효하지 않거나 존재하지 않을 경우 예외 발생.
 */
template <typename R, size_t N> void
set_value(char (&val)[N], const R &obj, const std::string &name)
{
  if (is_valid(val, obj, name) == false)
    throw std::pair<bool, std::string>{false, "Invalid or missing field: " + name};

  val << obj[name.c_str()];
}

/**
 * @brief JSON 필드 값을 `long long` 배열에 설정하는 함수.
 *
 * @tparam R JSON 객체의 타입.
 * @tparam N `long long` 배열의 크기.
 * @param val 값을 설정할 `long long` 배열 참조.
 * @param obj 데이터를 포함한 JSON 객체.
 * @param name JSON 필드 이름.
 * @throws std::pair<bool, std::string> 필드가 유효하지 않거나 존재하지 않을 경우 예외 발생.
 * @throws std::pair<bool, std::string> 배열 요소가 `long long` 타입이 아닌 경우 예외 발생.
 */
template <typename R, size_t N> void
set_value(long long (&val)[N], const R &obj, const std::string &name)
{
  if (is_valid(val, obj, name) == false)
    throw std::pair<bool, std::string>{false, "Invalid or missing field: " + name};

  const rapid_value &arr = obj[name.c_str()].GetArray();
  size_t arr_size = std::min(static_cast<size_t>(arr.Size()), N);

  for (size_t index = 0; index < arr_size; ++index)
  {
    if (arr[index].IsInt64() == false)
      throw std::pair<bool, std::string>{false, "Invalid type in field: " + name + " (expected long long array)"};
    val[index] = arr[index].GetInt64();
  }
}

/**
 * @brief JSON 필드 값을 2차원 `char` 배열에 설정하는 함수.
 *
 * @tparam R JSON 객체의 타입.
 * @tparam N 2차원 배열의 첫 번째 크기.
 * @tparam M 2차원 배열의 두 번째 크기.
 * @param val 값을 설정할 2차원 `char` 배열 참조.
 * @param obj 데이터를 포함한 JSON 객체.
 * @param name JSON 필드 이름.
 * @throws std::pair<bool, std::string> 필드가 유효하지 않거나 존재하지 않을 경우 예외 발생.
 * @throws std::pair<bool, std::string> 배열 요소가 문자열이 아닌 경우 예외 발생.
 */
template <typename R, size_t N, size_t M> void
set_value(char (&val)[N][M], const R &obj, const std::string &name)
{
  if (is_valid(val, obj, name) == false)
    throw std::pair<bool, std::string>{false, "Invalid or missing field: " + name};

  const rapid_value &arr = obj[name.c_str()];
  size_t arr_size = std::min(static_cast<size_t>(arr.Size()), N);

  memset(val, 0x00, sizeof(val));

  for (size_t index = 0; index < arr_size; ++index)
  {
    if (arr[index].IsString() == false)
      throw std::pair<bool, std::string>{false, "Invalid type in field: " + name + " (expected string array)"};
    strncpy(val[index], arr[index].GetString(), M - 1);
  }
}

/// optional /////////////////////////////////////////////////////////////////////////
template<typename R, typename T> void
set_value(Optional<std::vector<T>> &vals_opt, const R &obj, const std::string &name)
{
  vals_opt = nullopt;
  if (obj.HasMember(name.c_str()) == false)
    return;

  if (obj[name.c_str()].IsArray() == false)
    throw std::string("Not found or Invalid type " + name);

  const auto &items = obj[name.c_str()];

  std::vector<T> vals;
  vals.resize(items.Size());
  for (rapidjson::SizeType index = 0; index < items.Size(); ++index)
    vals[index] << items[index];

  vals_opt = std::move(vals);
}

/**
 * @brief JSON 문자열을 Optional<std::string>으로 변환하는 함수
 * 
 * @tparam R JSON 객체 타입
 * @param val_opt 값을 저장할 Optional 문자열
 * @param obj JSON 객체
 * @param name JSON 필드 이름
 * @throws std::string 필드가 문자열이 아닌 경우 예외 발생
 * 
 * @details
 * - 필드가 존재하지 않는 경우 nullopt 설정
 * - 필드가 존재하는 경우 JSON 문자열을 std::string으로 변환
 */
template<typename R> void
set_value(Optional<std::string> &val_opt, const R &obj, const std::string &name)
{
  val_opt = nullopt;
  if (!obj.HasMember(name.c_str()))
    return;

  if (!obj[name.c_str()].GetString())
    throw std::string("Not found or Invalid type ") + name;

  val_opt = std::string(obj[name.c_str()].GetString());
}

/**
 * @brief JSON 정수를 Optional<int32_t>로 변환하는 함수
 * 
 * @tparam R JSON 객체 타입
 * @param val_opt 값을 저장할 Optional 정수
 * @param obj JSON 객체
 * @param name JSON 필드 이름
 * @throws std::string 필드가 정수가 아닌 경우 예외 발생
 * 
 * @details
 * - 필드가 존재하지 않는 경우 nullopt 설정
 * - 필드가 존재하는 경우 JSON 정수를 int32_t로 변환
 */
template<typename R> void
set_value(Optional<int32_t> &val_opt, const R &obj, const std::string &name)
{
  val_opt = nullopt;
  if (!obj.HasMember(name.c_str()))
    return;

  if (!obj[name.c_str()].IsInt())
    throw std::string("Not found or Invalid type ") + name;

  val_opt = obj[name.c_str()].GetInt();
}

/**
 * @brief JSON 정수를 Optional<int64_t>로 변환하는 함수
 * 
 * @tparam R JSON 객체 타입
 * @param val_opt 값을 저장할 Optional 정수
 * @param obj JSON 객체
 * @param name JSON 필드 이름
 * @throws std::string 필드가 64비트 정수가 아닌 경우 예외 발생
 * 
 * @details
 * - 필드가 존재하지 않는 경우 nullopt 설정
 * - 필드가 존재하는 경우 JSON 정수를 int64_t로 변환
 */
template<typename R> void
set_value(Optional<int64_t> &val_opt, const R &obj, const std::string &name)
{
  val_opt = nullopt;
  if (!obj.HasMember(name.c_str()))
    return;

  if (!obj[name.c_str()].IsInt64())
    throw std::string("Not found or Invalid type ") + name;

  val_opt = obj[name.c_str()].GetInt64();
}
