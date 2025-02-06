/*
 * MJsonValue.h
 *
 *  Created on: 2024. 12. 29.
 *      Author: tys
 */

#pragma once

//////////////////////////////////////////////////////////////////////////////
// MJsonValue 구현부
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief MJsonValue의 복사 생성자
 * @param val 복사할 MJsonValue의 shared_ptr
 * @throw std::runtime_error val이 nullptr이거나 MJsonValue 타입이 아닌 경우
 * @details 다른 MJsonValue 객체로부터 새로운 MJsonValue를 생성합니다.
 */
inline
MJsonValue::MJsonValue(std::shared_ptr<MJsonValue> val)
{
  val->exception();

  if (val == nullptr)
    throw std::runtime_error(path_ + name_ + ": val is nullptr");

  if (val->is_value() == false)
    throw std::runtime_error(path_ + name_ + ": expected MJsonValue");

  path_       = val->path_;
  name_       = val->name_;
  value_      = val->value_;
  value_type_ = val->value_type_;
  type_       = val->type_;
  exception_  = val->exception_;
}

/**
 * @brief 객체의 특정 멤버를 배열로 변환하는 메서드
 * @param name 접근할 멤버의 이름
 * @return 멤버에 대한 MJsonArray 참조
 * @throw std::runtime_error 객체가 MJsonObject 타입이 아니거나 멤버가 배열이 아닌 경우
 * @details 객체의 특정 멤버를 MJsonArray 타입으로 변환하여 반환합니다.
 */
//inline const MJsonArray &
//MJsonValue::as_array(const std::string &name) const
//{
//  exception();
//  auto &self = get_self<MJsonObject>();
//  return *((std::static_pointer_cast<const MJsonArray>(self.by(name))).get());
//}

inline const MJsonArray &
MJsonValue::as_array() const
{
  exception();
  return get_self<MJsonArray>();
}

inline const MJsonObject &
MJsonValue::as_object() const
{
  exception();
  return get_self<MJsonObject>();
}

/**
 * @brief 배열의 특정 요소를 객체로 변환하는 메서드
 * @param index 접근할 요소의 인덱스
 * @return 요소에 대한 MJsonObject 참조
 * @throw std::runtime_error 객체가 MJsonArray 타입이 아니거나 요소가 객체가 아닌 경우
 * @details 배열의 특정 인덱스 위치의 요소를 MJsonObject 타입으로 변환하여 반환합니다.
 */
//inline const  MJsonObject &
//MJsonValue::as_object(size_t index) const
//{
//  exception();
//  auto &self = get_self<MJsonArray>();
//  return *((std::static_pointer_cast<const MJsonObject>(self.at(index))).get());
//}

/**
 * @brief 객체의 멤버에 접근하는 연산자
 * @param name 접근할 멤버의 이름
 * @return 멤버에 대한 MJsonObject 참조
 * @throw std::runtime_error 객체가 MJsonObject 타입이 아닌 경우
 * @details 객체의 특정 멤버에 대한 참조를 반환합니다.
 */
inline const MJsonValue &
MJsonValue::operator[](const std::string &name) const
{
  exception();
  entity_ = std::static_pointer_cast<const MJsonValue>(get_self<MJsonObject>().by(name));
  return *(entity_.get());
}

/**
 * @brief 배열의 요소에 접근하는 연산자
 * @param index 접근할 요소의 인덱스
 * @return 요소에 대한 MJsonArray 참조
 * @throw std::runtime_error 객체가 MJsonArray 타입이 아닌 경우
 * @details 배열의 특정 인덱스 위치의 요소에 대한 참조를 반환합니다.
 */
inline const MJsonValue &
MJsonValue::operator[](size_t index) const
{
  exception();
  entity_ = std::static_pointer_cast<const MJsonValue>(get_self<MJsonArray>().at(index));
  return *(entity_.get());
}

template<typename T> const T &
MJsonValue::required(const std::string &name, std::function<void(const T &)> handler) const
{
  exception();
  auto &self = get_self<MJsonObject>();
  handler(*(std::static_pointer_cast<const MJsonValue>(self.by(name))).get());
  return self;
}

template<typename T> const T &
MJsonValue::required(size_t index, std::function<void(const T &)> handler) const
{
  exception();
  auto &self = get_self<MJsonArray>();
  handler(*(std::static_pointer_cast<const MJsonValue>(self.at(index))).get());
  return self;
}

template<typename T> const MJsonArray &
MJsonValue::required_for(const std::string &name, std::function<void(const T &, size_t)> handler) const
{
  exception();
  auto &self = get_self<MJsonObject>();
  auto &child = *(std::static_pointer_cast<const MJsonArray>(self.by(name))).get();
  for (size_t index = 0; index < child.size(); ++index)
    handler(child, index);

  return child;
}

template<typename T> const MJsonArray &
MJsonValue::required_for(size_t index, std::function<void(const T &, size_t)> handler) const
{
  exception();
  auto &self = get_self<MJsonArray>();
  auto &child = *(std::static_pointer_cast<const MJsonArray>(self.at(index))).get();
  for (size_t cindex = 0; cindex < child.size(); ++cindex)
    handler(child, cindex);

  return child;
}

//template<typename T> const T &
//MJsonValue::required(std::function<void(const T &)> handler) const
//{
//  exception();
//  handler(get_self<T>());
//  return get_self<T>();
//}
//
//inline const MJsonArray &
//MJsonValue::required_for(std::function<void(const MJsonArray &, size_t)> handler) const
//{
//  exception();
//  const auto &self = get_self<MJsonArray>();
//  for (size_t index = 0; index < self.size(); ++index)
//    handler(self, index);
//
//  return self;
//}

//template<typename T> void
//MJsonValue::optional(std::function<void(const T &)> handler,
//                     std::function<void()> alternative) const
//{
//  if (exception_ == true) alternative();
//  else                    handler(get_self<T>());
//}

template<typename T> void
MJsonValue::optional(const std::string &name,
                     std::function<void(const T &)> handler,
                     std::function<void()> alternative) const
{
  exception();
  auto &self = get_self<MJsonObject>();
  auto it = self.members.find(name);
  if (it == self.members.end())
  {
    if (alternative != nullptr) alternative();
    return;
  }

  handler(*(it->second.get()));
}

template<typename T> void
MJsonValue::optional(size_t index,
                     std::function<void(const T &)> handler,
                     std::function<void()> alternative) const
{
  exception();
  auto &self = get_self<MJsonArray>();
  if (self.elements.size() >= index)
  {
    if (alternative != nullptr) alternative();
    return;
  }

  handler(self.elements[index]);
}

inline void
MJsonValue::optional_for(const std::string &name,
                         std::function<void(const MJsonArray &, size_t)> handler,
                         std::function<void()> alternative) const
{
  exception();
  auto &self = get_self<MJsonObject>();
  auto it = self.members.find(name);
  if (it == self.members.end())
  {
    if (alternative != nullptr) alternative();
    return;
  }

  auto &child = *(std::static_pointer_cast<const MJsonArray>(it->second)).get();
  for (size_t index = 0; index < child.size(); ++index)
    handler(child, index);
}

inline void
MJsonValue::optional_for(size_t index,
                         std::function<void(const MJsonArray &, size_t)> handler,
                         std::function<void()> alternative) const
{
  exception();
  auto &self = get_self<MJsonArray>();
  if (index >= self.size())
  {
    if (alternative != nullptr) alternative();
    return;
  }

  auto &child = *(std::static_pointer_cast<const MJsonArray>(self.elements[index])).get();
  for (size_t index = 0; index < child.size(); ++index)
    handler(child, index);
}
//inline void
//MJsonValue::optional_for(std::function<void(const MJsonArray &, size_t)> handler,
//                         std::function<void()> alternative) const
//{
//  if (exception_ == true) { alternative(); return; }
//  const auto &self = get_self<MJsonArray>();
//  for (size_t index = 0; index < self.size(); ++index)
//    handler(self, index);
//}

template<typename T> const T &
MJsonValue::get_self_const() const
{
  if (std::is_same<T, MJsonValue>::value == true)
    return *static_cast<const T*>(this);
  //    if (is_value() == false)
  //      throw std::runtime_error(path_ + name_ + ": expected MJsonValue, got " + type_str());
  if (std::is_same<T, MJsonArray>::value == true)
    if (is_array() == false)
      throw std::runtime_error(path_ + name_ + ": expected MJsonArray, got " + type_str());
  if (std::is_same<T, MJsonObject>::value == true)
    if (is_object() == false)
      throw std::runtime_error(path_ + name_ + ": expected MJsonObject, got " + type_str());
  return *static_cast<const T*>(this);
}

inline std::shared_ptr<MJsonValue>
MJsonValue::take_ptr() const
{
  exception();

  if (is_value())
  {
    auto &self = const_cast<MJsonValue &>(get_self<MJsonValue>());
    return std::make_shared<MJsonValue>(self.take_impl());
  }

  if (is_object())
  {
    auto &self = const_cast<MJsonObject &>(get_self<MJsonObject>());
    return std::make_shared<MJsonObject>(self.take_impl());
  }

  auto &self = const_cast<MJsonArray &>(get_self<MJsonArray>());
  return std::make_shared<MJsonArray>(self.take_impl());
}

/**
 * @brief 배열에서 특정 인덱스의 요소 존재 여부를 확인하는 메서드
 * @param index 확인할 인덱스
 * @return 요소 존재 여부
 * @throw std::runtime_error 객체가 MJsonArray 타입이 아닌 경우
 * @details 배열에서 주어진 인덱스의 요소가 존재하는지 확인합니다.
 */
inline bool
MJsonValue::has(size_t index) const
{
  exception();

  if (is_array() == false)
    throw std::runtime_error(path_ + name_ + ": expected MJsonArray, got " + type_str());
  return has_impl(index);
}

/**
 * @brief 객체에서 특정 이름의 멤버 존재 여부를 확인하는 메서드
 * @param name 확인할 멤버의 이름
 * @return 멤버 존재 여부
 * @throw std::runtime_error 객체가 MJsonObject 타입이 아닌 경우
 * @details 객체에서 주어진 이름의 멤버가 존재하는지 확인합니다.
 */
inline bool
MJsonValue::has(const std::string &name) const
{
  exception();

  if (is_object() == false)
    throw std::runtime_error(path_ + name_ + ": expected MJsonObject, got " + type_str());
  return has_impl(name);
}

inline size_t
MJsonValue::size() const { return 0; }
