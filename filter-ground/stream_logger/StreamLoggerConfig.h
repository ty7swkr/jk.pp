/*
 * StreamLoggerConfig.h
 *
 *  Created on: 2024. 12. 13.
 *      Author: tys
 */

#pragma once

#include <extra/SysDateTime.h>
#include <extra/Singleton.h>
#include <extra/helper.h>

/**
 * @brief StreamLoggerConfig
 * @details
 * - StreamLoggerConfig는 타입정의와 기본적인 로거 설정을 저장합니다.
 * - Type는 로그 출력시 선택되므로 Config에는 포함되지 않습니다.
 */
class StreamLoggerConfig
{
public:
  struct Type
  {
  public:
    enum
    { APPLICATION = 1, TRANSACTION = 2, SENSING = 3 };

    static std::string str(const int &value);
  };

  struct Level
  {
  public:
    enum { ERROR = 1, WARN, INFO, DEBUG };
    bool is_valid(const int &value) const;
    static std::string str(const int &value);

    bool error  = true;
    bool warn   = true;
    bool info   = true;
    bool debug  = true;

  } level;

  std::string app_name;
  size_t      queue_size  = 10000;
  bool        pretty      = true;
};

inline std::string
StreamLoggerConfig::Type::str(const int &value)
{
  switch (value)
  {
    case StreamLoggerConfig::Type::APPLICATION: return "application";
    case StreamLoggerConfig::Type::TRANSACTION: return "transaction";
    case StreamLoggerConfig::Type::SENSING    : return "sensing";
  }
  return "unknown";
}

inline std::string
StreamLoggerConfig::Level::str(const int &value)
{
  switch (value)
  {
    case StreamLoggerConfig::Level::INFO : return "info";
    case StreamLoggerConfig::Level::WARN : return "warn";
    case StreamLoggerConfig::Level::ERROR: return "error";
    case StreamLoggerConfig::Level::DEBUG: return "debug";
  }
  return "none";
}

inline bool
StreamLoggerConfig::Level::is_valid(const int &value) const
{
  switch (value)
  {
    case StreamLoggerConfig::Level::INFO : return info;
    case StreamLoggerConfig::Level::WARN : return warn;
    case StreamLoggerConfig::Level::ERROR: return error;
    case StreamLoggerConfig::Level::DEBUG: return debug;
  }
  return true;
}


