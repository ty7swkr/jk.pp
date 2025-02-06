/*
 * media_content.h
 *
 *  Created on: 2024. 12. 13.
 *      Author: tys
 */

#pragma once

#include <extra/Optional.h>
#include <extra/rapidjson_helper.h>
#include <string>
#include <vector>

struct media_content_t
{
  Optional<int32_t>     contentType    = 0;
  Optional<int32_t>     contentSize    = 0;
  Optional<std::string> contentUrl;
  Optional<int32_t>     encryptFlag    = 0;
  Optional<std::string> decodingKey;
};

template<typename RAPID_OBJECT> RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const std::vector<media_content_t> &values, rapidjson::Document::AllocatorType &al)
{
  rapid_value array(rapidjson::kArrayType);

  for (auto &value : values)
  {
    rapid_value sub(rapidjson::kObjectType);
    add_member(sub, "contentType",    value.contentType,    al);
    add_member(sub, "contentSize",    value.contentSize,    al);
    add_member(sub, "contentUrl",     value.contentUrl,     al);
    add_member(sub, "encryptFlag",    value.encryptFlag,    al);
    add_member(sub, "decodingKey",    value.decodingKey,    al);
    array.PushBack(sub, al);
  }

  obj.AddMember(rapid_value(name.c_str(), al).Move(), array, al);
  return obj;
}

template<typename RAPID_OBJECT> RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const Optional<std::vector<media_content_t>> &values, rapidjson::Document::AllocatorType &al)
{
  if (values == nullopt)
    return obj;

  return add_member(obj, name, values.value(), al);
}

template<typename RAPID_OBJECT> void
set_value(media_content_t &value, RAPID_OBJECT &obj)
{
  set_value(value.contentType,  obj, "contentType");
  set_value(value.contentSize,  obj, "contentSize");
  set_value(value.contentUrl,   obj, "contentUrl");
  set_value(value.encryptFlag,  obj, "encryptFlag");
  set_value(value.decodingKey,  obj, "decodingKey");
}

template<typename RAPID_OBJECT> void
set_value(std::vector<media_content_t> &values, RAPID_OBJECT &obj, const std::string &name)
{
  if (!obj.HasMember(name.c_str()))
    throw std::string("Not found or Invalid type " + name);

  if (!obj[name.c_str()].IsArray())
    throw std::string("Not found or Invalid type " + name);

  const auto &items = obj[name.c_str()];
  for (rapidjson::SizeType index = 0; index < items.Size(); ++index)
  {
    values.emplace_back();
    set_value(values.back(), items[index]);
  }
}

template<typename RAPID_OBJECT> void
set_value(Optional<std::vector<media_content_t>> &values_opt, RAPID_OBJECT &obj, const std::string &name)
{
  values_opt = nullopt;
  if (obj.HasMember(name.c_str()) == false)
    return;

  std::vector<media_content_t> media_content;
  set_value(media_content, obj, name);
  values_opt = media_content;
}


