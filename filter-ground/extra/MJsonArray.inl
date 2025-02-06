/*
 * MJsonArray.h
 *
 *  Created on: 2024. 12. 29.
 *      Author: tys
 */

//////////////////////////////////////////////////////////////////////////////
// MJsonArray 구현부
//////////////////////////////////////////////////////////////////////////////
/**
 * @brief MJsonArray의 변환 생성자
 * @param val 변환할 MJsonValue의 shared_ptr
 * @throw std::runtime_error val이 nullptr이거나 MJsonArray 타입이 아닌 경우
 * @details MJsonValue 객체를 MJsonArray로 변환하여 새로운 배열을 생성합니다.
 */
inline
MJsonArray::MJsonArray(std::shared_ptr<MJsonValue> &&val) : MJsonValue(MJsonValue::ARRAY)
{
  val->exception();

  if (val == nullptr)
    throw std::runtime_error(path_ + name_ + ": Invalid argument: null MJsonValue pointer");

  if (val->is_array() == false)
    throw std::runtime_error(path_ + name_ + ": expected MJsonArray, got " + val->type_str());

  path_       .swap(val->path_);
  name_       .swap(val->name_);
  value_      .swap(val->value_);
  value_type_ = val->value_type_;
  type_       = val->type_;
  exception_  = val->exception_;

  auto rhs = std::static_pointer_cast<MJsonArray>(val);
  // 찾은 멤버를 이동
  elements.swap(rhs->elements);
}

inline
MJsonArray::MJsonArray(MJsonArray &&val)
: MJsonValue(MJsonValue::ARRAY)
{
  val.exception();

  if (val.is_array() == false)
    throw std::runtime_error(path_ + name_ + ": expected MJsonArray, got " + val.type_str());

  path_       .swap(val.path_);
  name_       .swap(val.name_);
  value_      .swap(val.value_);
  value_type_ = val.value_type_;
  type_       = val.type_;
  exception_  = val.exception_;

  // 찾은 멤버를 이동
  elements.swap(val.elements);
}

/**
 * @brief MJsonArray의 대입 연산자
 * @param rhs 대입할 MJsonArray 객체
 * @return 대입된 MJsonArray 참조
 * @throw std::runtime_error rhs가 배열이 아닌 경우
 * @details 다른 MJsonArray 객체의 내용을 현재 객체로 복사합니다.
 */
inline MJsonArray &
MJsonArray::operator=(const MJsonArray& rhs)
{
  rhs.exception();

  if (this == &rhs) return *this;

  if (rhs.is_object() == true)
    throw std::runtime_error(rhs.path_ + rhs.name_ + ": expected Array, got " + rhs.type_str());

  path_       = rhs.path_;
  name_       = rhs.name_;
  value_      = rhs.value_;
  value_type_ = rhs.value_type_;
  type_       = rhs.type_;
  exception_  = rhs.exception_;

  if (rhs.is_array() == false)
    return *this;

  if (exception_ == true)
    return *this;

  // deep copy
  for (const auto &element : rhs.elements)
  {
    if (element->exception_ == true) continue;
    if (element->is_array() == true)
    {
      elements.emplace_back(
          std::make_shared<MJsonArray>(*std::static_pointer_cast<MJsonArray>(element)));
      continue;
    }
    if (element->is_object() == true)
    {
      elements.emplace_back(
          std::make_shared<MJsonObject>(*std::static_pointer_cast<MJsonObject>(element)));
      continue;
    }

    elements.emplace_back(std::make_shared<MJsonValue>(*element));
  }

  return *this;
}

/**
 * @brief 배열에 새로운 요소를 추가하는 템플릿 메서드
 * @param element 추가할 요소
 * @tparam T 요소의 타입
 * @details 배열의 끝에 새로운 요소를 추가합니다.
 */
template<typename T> inline void
MJsonArray::add(const T &element)
{
  elements.push_back(std::make_shared<T>(element));
}

/**
 * @brief 배열의 특정 인덱스 요소에 접근하는 메서드
 * @param index 접근할 요소의 인덱스
 * @return 요소에 대한 MJsonValue의 shared_ptr
 * @throw std::runtime_error 인덱스가 범위를 벗어난 경우
 * @details 배열의 특정 인덱스 위치의 요소에 대한 포인터를 반환합니다.
 */
inline std::shared_ptr<MJsonValue>
MJsonArray::at(size_t index) const
{
  exception();

  if (index >= elements.size())
  {
    auto value = std::make_shared<MJsonArray>();
    value->path_       = path_ + name_ + "/";
    value->name_       = "[" + std::to_string(index) + "]";
    value->type_       = type_;
    value->value_type_ = value_type_;
    value->exception_  = true;
    return value;
  }
  return elements[index];
}

//inline MJsonArray
//MJsonArray::take()
//{
//  exception();
//  return std::move(*this);
//  MJsonArray &self = const_cast<MJsonArray &>(*this);
//  MJsonArray rhs;
//  rhs.path_       .swap(self.path_);
//  rhs.name_       .swap(self.name_);
//  rhs.value_      .swap(self.value_);
//  rhs.value_type_ = value_type_;
//  rhs.type_       = type_;
//  rhs.exception_  = exception_;
//
//  // 찾은 멤버를 이동
//  rhs.elements.swap(self.elements);
//  return rhs;
//}
