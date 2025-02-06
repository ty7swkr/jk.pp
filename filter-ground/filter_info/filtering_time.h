/*
 * filtering_time.h
 *
 *  Created on: 2024. 12. 18.
 *      Author: tys
 */

#pragma once

#include <extra/rapidjson_helper.h>
#include <extra/Optional.h>
#include <extra/Expected.h>
#include <string>
#include <map>

struct filtering_time_t
{
  std::string filterName;
  int64_t     startTime;  // time_t + millisec
  int64_t     endTime;
};

using filtering_time_objs = std::map<std::string, filtering_time_t>;

template<typename RAPID_OBJECT> RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const filtering_time_t &value, rapidjson::Document::AllocatorType &al)
{
  rapid_value sub(rapidjson::kObjectType);

  add_member(sub, "filterName", value.filterName, al);
  add_member(sub, "startTime",  value.startTime, al);
  add_member(sub, "endTime",    value.endTime,   al);
  return obj;
}

template<typename RAPID_OBJECT> void
set_value(filtering_time_objs &filtering_times, const RAPID_OBJECT &obj, const std::string &name)
{
  if (!obj.HasMember(name.c_str()))
    throw std::string("Not found or Invalid type ") + name;

  if (!obj[name.c_str()].IsArray())
    throw std::string("Not found or Invalid type ") + name;

  const auto &items = obj[name.c_str()];
  for (rapidjson::SizeType index = 0; index < items.Size(); ++index)
  {
    const rapid_value &item = items[index];

    filtering_time_t &filtering_time = filtering_times[name];
    set_value(filtering_time.filterName,  item, "filterName");
    set_value(filtering_time.startTime,   item, "startTime");
    set_value(filtering_time.endTime,     item, "endTime");
  }
}

