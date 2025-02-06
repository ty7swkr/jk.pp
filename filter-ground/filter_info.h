/*
 * filter_info.h
 *
 *  Created on: 2024. 12. 13.
 *      Author: tys
 */

#pragma once

#include <filter_info/customer_info.h>
#include <filter_info/message_info.h>
#include <filter_info/result_info.h>
#include <extra/rapidjson_helper.h>
#include <extra/Expected.h>

/**
example
test_filter_json.h 소스 참조
*/

struct filter_info_t
{
  message_info_t  messageInfo;
  customer_info_t customerInfo;
  result_info_t   resultInfo;
};

inline std::string
to_json(const filter_info_t &info, bool pretty = false)
{
  rapidjson::Document doc(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType &al = doc.GetAllocator();

  add_member(doc, "messageInfo",  info.messageInfo,  al);
  add_member(doc, "customerInfo", info.customerInfo, al);
  add_member(doc, "resultInfo",   info.resultInfo,   al);

  return to_json(doc, pretty);
}

inline Expected<filter_info_t, std::string> // success, error str
from_filter_info_json(const std::string &json)
{
  rapidjson::Document doc;

  if (doc.Parse(json.c_str()).HasParseError() == true)
    return make_unexpected("JSON parse error!");

  filter_info_t value;

  if (doc.HasMember("messageInfo") == false)
    return make_unexpected("Not found messageInfo");

  if (doc["messageInfo"].IsObject() == false)
    return make_unexpected("Invalid type messageInfo");

  if (doc.HasMember("customerInfo") == false)
    return make_unexpected("Not found customerInfo");

  if (doc["customerInfo"].IsObject() == false)
    return make_unexpected("Invalid type customerInfo");

  if (doc.HasMember("resultInfo") == false)
    return make_unexpected("Not found resultInfo");

  if (doc["resultInfo"].IsObject() == false)
    return make_unexpected("Invalid type resultInfo");

  try
  {
    set_value(value.messageInfo,   doc["messageInfo"   ]);
    set_value(value.customerInfo,  doc["customerInfo"  ]);
    set_value(value.resultInfo,    doc["resultInfo"    ]);
  }
  catch (const std::string &e)
  {
    return make_unexpected(e);
  }

  return value;
}
