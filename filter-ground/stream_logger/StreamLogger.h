/*
 * StreamLogger.h
 *
 *  Created on: 2024. 12. 13.
 *      Author: tys
 */

#pragma once

#include <stream_logger/StreamLoggerHandler.h>

/**
 * @brief StreamLogger
 * @details
 * - StreamLogger는 StreamLoggerHandler를 생성하여 로그를 작성할 수 있게 합니다.
 * - StreamLogger은 StreamLoggerHandler builder 역할을 합니다.
 */
class StreamLogger
{
public:
  StreamLogger(const char *filename, const int &line, const char *function,
               const StreamLoggerConfig &config, StreamLoggerWriter &writer)
  : file_(filename), line_(line), func_(function), config_(config), writer_(writer) {}

  StreamLoggerHandler &info () { return make_handler(StreamLoggerConfig::Level::INFO  );  }
  StreamLoggerHandler &warn () { return make_handler(StreamLoggerConfig::Level::WARN  );  }
  StreamLoggerHandler &error() { return make_handler(StreamLoggerConfig::Level::ERROR );  }
  StreamLoggerHandler &debug() { return make_handler(StreamLoggerConfig::Level::DEBUG );  }

  StreamLogger        &ap   () { return this->application(); }
  StreamLogger        &tr   () { return this->transaction(); }
  StreamLogger        &ss   () { return this->sensing();     }

  StreamLogger        &application () { type_ = StreamLoggerConfig::Type::APPLICATION; return *this; }
  StreamLogger        &transaction () { type_ = StreamLoggerConfig::Type::TRANSACTION; return *this; }
  StreamLogger        &sensing     () { type_ = StreamLoggerConfig::Type::SENSING;     return *this; }

protected:
  StreamLoggerHandler &make_handler(int level)
  {
    handler_ = std::make_shared<StreamLoggerHandler>(type_, level, file_, line_, func_, config_, writer_);
    return *(handler_.get());
  }

  std::shared_ptr<StreamLoggerHandler> handler_;

protected:
  int         type_ = StreamLoggerConfig::Type::APPLICATION;
  const char *file_ = nullptr;
  const int   line_ = 0;
  const char *func_ = nullptr;

protected:
  StreamLoggerConfig config_;
  StreamLoggerWriter &writer_;
};

