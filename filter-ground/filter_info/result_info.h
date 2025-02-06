/*
 * result_info.h
 *
 *  Created on: 2024. 12. 13.
 *      Author: tys
 */

#pragma once

#include <filter_info/filtering_time.h>
#include <extra/rapidjson_helper.h>
#include <extra/Optional.h>
#include <extra/Expected.h>
#include <string>

/**
 * 필터정보 규격서에 따라서 필수 정보와 옵션으로 나누어 져 있어서 아래와 같이
 * Optional(c++17 기능 차용)을 사용하였습니다.
 *
 * 생성시 초기화
 * has_value가 false
 * Optional<std::string> var;
 *
 * has_value가 true
 * Optional<std::string> var = "abcd";
 * Optional<std::string> var("abcd");
 * Optional<int32_t> var = 3;
 * Optional<int32_t> var(3);
 *
 * Optional<std::string> var; // 빈값. has_value가 false
 * var = "abcd"; // has_value true
 *
 * Optional<int32_t> var;
 * var = 1234;
 *
 * 값 출력하기
 * if (var.has_value() == false)
 *   std::cout << "no value" << std::endl;
 * else
 *   std::cout << var.value() << std::endl;
 *
 */

struct result_info_t
{
  int32_t               smppResult      = 0;
  int32_t               resultCode      = 0;
  int32_t               reasonCode      = 0;
  std::string           spamPattern1;
  Optional<std::string> spamPattern2;
  Optional<std::string> spamPattern3;
  Optional<std::string> imageFileName;
  int64_t               filterStartTime = 0;
  int64_t               filterEndTime   = 0;
  filtering_time_objs   filteringTime;
};

template<typename RAPID_OBJECT> void
add_member(RAPID_OBJECT &obj, const std::string &name, const filtering_time_objs &filtering_times, rapidjson::Document::AllocatorType &al);

template<typename RAPID_OBJECT> RAPID_OBJECT &
add_member(RAPID_OBJECT &obj, const std::string &name, const result_info_t &value, rapidjson::Document::AllocatorType &al)
{
  rapid_value sub(rapidjson::kObjectType);

  add_member(sub, "smppResult",       value.smppResult,     al);
  add_member(sub, "resultCode",       value.resultCode,     al);
  add_member(sub, "reasonCode",       value.reasonCode,     al);
  add_member(sub, "spamPattern1",     value.spamPattern1,   al);
  add_member(sub, "spamPattern2",     value.spamPattern2,   al);
  add_member(sub, "spamPattern3",     value.spamPattern3,   al);
  add_member(sub, "imageFileName",    value.imageFileName,   al);
  add_member(sub, "filterStartTime",  value.filterStartTime,al);
  add_member(sub, "filterEndTime",    value.filterEndTime,  al);
  add_member(sub, "filteringTime",    value.filteringTime,  al);

  obj.AddMember(rapid_value(name.c_str(), al).Move(), sub, al);
  return obj;
}

template<typename RAPID_OBJECT> void
add_member(RAPID_OBJECT &obj, const std::string &name, const filtering_time_objs &filtering_times, rapidjson::Document::AllocatorType &al)
{
  rapid_value array(rapidjson::kArrayType);
  for (auto &filtering_time : filtering_times)
  {
    rapid_value sub(rapidjson::kObjectType);
    add_member(sub, "filterName", filtering_time.first,             al);
    add_member(sub, "startTime",  filtering_time.second.startTime,  al);
    add_member(sub, "endTime",    filtering_time.second.endTime,    al);
    array.PushBack(sub, al);
  }
  obj.AddMember(rapid_value(name.c_str(), al).Move(), array, al);
}

template<typename RAPID_OBJECT> void
set_value(result_info_t &value, RAPID_OBJECT &obj)
{
  set_value(value.smppResult,     obj,  "smppResult");
  set_value(value.resultCode,     obj,  "resultCode");
  set_value(value.reasonCode,     obj,  "reasonCode");
  set_value(value.spamPattern1,   obj,  "spamPattern1");
  set_value(value.spamPattern2,   obj,  "spamPattern2");
  set_value(value.spamPattern3,   obj,  "spamPattern3");
  set_value(value.filterStartTime,obj,  "filterStartTime");
  set_value(value.filterEndTime,  obj,  "filterEndTime");
  set_value(value.filteringTime,  obj,  "filteringTime");
}


