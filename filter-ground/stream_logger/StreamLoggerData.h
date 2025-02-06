/*
 * StreamLoggerData.h
 *
 *  Created on: 2025. 1. 25.
 *      Author: tys
 */

#pragma once

#include <stream_logger/StreamLoggerConfig.h>
#include <extra/rapidjson_helper.h>

class StreamLoggerData
{
public:
  StreamLoggerData() {}

  bool  pretty= false;
  int   type  = StreamLoggerConfig::Type::APPLICATION;
  int   level = StreamLoggerConfig::Level::INFO;
  SysDateTime create_time;
  std::string location;
  std::stringstream message;

  std::string to_json() const;
};

inline std::string
StreamLoggerData::to_json() const
{
//  {
//      "logType": "application",
//      "logLevel": "info",
//      "createTime": "2025-01-22 22:12:12.135",
//      "logData": {
//          "location": "[auth-filter]#09:0x537FE640:FilterWorker.cpp:34:run",
//          "message": "Stop FilterWorker: #09"
//      }
//  }

  rapidjson::Document doc(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType &al = doc.GetAllocator();

  add_member(doc, "logType",    StreamLoggerConfig::Type ::str(type ),    al);
  add_member(doc, "logLevel",   StreamLoggerConfig::Level::str(level),    al);
  add_member(doc, "createTime", create_time.to_string("%Y-%m-%d %H:%M:%S.%L"),al);

  rapidjson::Value log_data(rapidjson::kObjectType);
  add_member(log_data, "location",  location,      al);
  add_member(log_data, "message",   message.str(), al);

  doc.AddMember(rapid_value("logData", al).Move(), log_data, al);

  rapidjson::StringBuffer buffer;
  if (pretty == true)
  {
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    writer.SetIndent(' ', 1);
    doc.Accept(writer);
  }
  else
  {
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);  // 일반 Writer 사용
    doc.Accept(writer);
  }
  return buffer.GetString();
}

