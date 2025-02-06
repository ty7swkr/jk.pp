/*
 * MJsonObject.inl
 *
 *  Created on: 2024. 12. 31.
 *      Author: tys
 */

#pragma once

//////////////////////////////////////////////////////////////////////////////
// MJsonObject 구현부
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief MJsonObject의 변환 생성자
 * @param val 변환할 MJsonValue의 shared_ptr
 * @throw std::runtime_error val이 nullptr이거나 MJsonObject 타입이 아닌 경우
 * @details MJsonValue 객체를 MJsonObject로 변환하여 새로운 객체를 생성합니다.
 */
inline
MJsonObject::MJsonObject(std::shared_ptr<MJsonValue> &&val) : MJsonValue(MJsonValue::TYPE::OBJECT)
{
  val->exception();

  if (val == nullptr)
    throw std::runtime_error(path_ + name_ + ": Invalid argument: null MJsonValue pointer");

  if (val->is_object() == false)
    throw std::runtime_error(path_ + name_ + ": expected MJsonObject, got " + val->type_str());

  path_       .swap(val->path_);
  name_       .swap(val->name_);
  value_      .swap(val->value_);
  value_type_ = val->value_type_;
  type_       = val->type_;
  exception_  = val->exception_;

  // 찾은 멤버를 이동
  members.swap(std::static_pointer_cast<MJsonObject>(val)->members);
}

inline
MJsonObject::MJsonObject(MJsonObject &&val)
: MJsonValue(MJsonValue::OBJECT)
{
  val.exception();

  if (val.is_object() == false)
    throw std::runtime_error(path_ + name_ + ": expected MJsonArray, got " + val.type_str());

  path_       .swap(val.path_);
  name_       .swap(val.name_);
  value_      .swap(val.value_);
  value_type_ = val.value_type_;
  type_       = val.type_;
  exception_  = val.exception_;

  // 찾은 멤버를 이동
  members.swap(val.members);
}

/**
 * @brief MJsonObject의 대입 연산자
 * @param rhs 대입할 MJsonObject 객체
 * @return 대입된 MJsonObject 참조
 * @throw std::runtime_error rhs가 객체가 아닌 경우
 * @details 다른 MJsonObject 객체의 내용을 현재 객체로 복사합니다.
 */
inline MJsonObject &
MJsonObject::operator=(const MJsonObject& rhs)
{
  exception();

  if (this == &rhs) return *this;

  if (rhs.is_array() == true)
    throw std::runtime_error(rhs.path_ + rhs.name_ + ": expected MJsonObject, got " + rhs.type_str());

  path_       = rhs.path_;
  name_       = rhs.name_;
  value_      = rhs.value_;
  value_type_ = rhs.value_type_;
  type_       = rhs.type_;
  exception_  = rhs.exception_;

  if (rhs.is_object() == false)
    return *this;

  if (exception_ == true)
    return *this;

  // deep copy
  for (const auto &member : rhs.members)
  {
    const std::string                 &name    = member.first;
    const std::shared_ptr<MJsonValue> &element = member.second;

    if (element->exception_ == true) continue;
    if (element->is_array() == true)
    {
      members.emplace(name, std::make_shared<MJsonArray>(*std::static_pointer_cast<MJsonArray>(element)));
      continue;
    }
    if (element->is_object() == true)
    {
      members.emplace(name, std::make_shared<MJsonObject>(*std::static_pointer_cast<MJsonObject>(element)));
      continue;
    }

    members.emplace(name, std::make_shared<MJsonValue>(*element));
  }

  return *this;
}

/**
 * @brief 객체에 새로운 멤버를 추가하는 템플릿 메서드
 * @param name 추가할 멤버의 이름
 * @param element 추가할 멤버의 값
 * @tparam T 멤버 값의 타입
 * @details 객체에 이름-값 쌍의 새로운 멤버를 추가합니다.
 */
template<typename T> inline void
MJsonObject::add(const std::string &name, const T &element)
{
  members.emplace(name, std::make_shared<T>(element));
}

/**
 * @brief 객체의 특정 이름을 가진 멤버에 접근하는 메서드
 * @param name 접근할 멤버의 이름
 * @return 멤버에 대한 MJsonValue의 shared_ptr
 * @throw std::runtime_error 해당 이름의 멤버가 없는 경우
 * @details 객체의 특정 이름을 가진 멤버에 대한 포인터를 반환합니다.
 */
inline std::shared_ptr<MJsonValue>
MJsonObject::by(const std::string &name) const
{
  exception();

  auto it = members.find(name);
  if (it == members.end())
  {
    auto value = std::make_shared<MJsonObject>();
    value->path_       = path_ + name_ + "/";
    value->name_       = name;
    value->type_       = type_;
    value->value_type_ = value_type_;
    value->exception_  = true;
    return value;
  }
  return it->second;
}

//inline MJsonObject
//MJsonObject::take()
//{
//  exception();
//  return std::move(*this);
//  MJsonObject &self = const_cast<MJsonObject &>(*this);
//  MJsonObject rhs;
//  rhs.path_       .swap(self.path_);
//  rhs.name_       .swap(self.name_);
//  rhs.value_      .swap(self.value_);
//  rhs.value_type_ = self.value_type_;
//  rhs.type_       = self.type_;
//  rhs.exception_  = self.exception_;
//
//  // 찾은 멤버를 이동
//  rhs.members.swap(self.members);
//  return rhs;
//}
