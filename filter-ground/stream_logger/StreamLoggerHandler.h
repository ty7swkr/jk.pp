/*
 * trace.h
 *
 *  Created on: 2020. 2. 11.
 *      Author: tys
 */

#pragma once

#include <stream_logger/StreamLoggerConfig.h>
#include <stream_logger/StreamLoggerWriter.h>
#include <stream_logger/StreamLoggerData.h>
#include <extra/ThreadUniqueIndexer.h>
#include <extra/helper.h>
#include <memory>
#include <iomanip>

/**
 * @brief StreamLoggerHandler
 * @details
 * - StreamLoggerHandler는 로그를 출력할 때 사용하는 최종 사용자 인터페이스 입니다.
 * - StreamLoggerHandler는 다양한 형식의 데이터를 스트림 형태로 출력할 수 있습니다.
 * - 로그 타입, 로그 레벨, 파일명, 라인번호, 함수명, 사용자 메세지를 StreamLoggerData에 저장하고
 *   StreamLoggerWriter로 전달합니다.
 */
class StreamLoggerHandler
{
public:
  class Delim
  {
  public:
    Delim(const std::string &delim = "") : delim(delim) {}
    std::string delim;
  };

  StreamLoggerHandler(const int  &type, const int &level,
                      const char *file, const int &line, const char *function,
                      const StreamLoggerConfig &config,
                      StreamLoggerWriter &writer)
  : config_(config), writer_(writer)
  {
    logging_ = config_.level.is_valid(level);
    if (logging_ == false)
      return;

    data_ = std::make_shared<StreamLoggerData>();
    data_->pretty = config.pretty;
    data_->type   = type;
    data_->level  = level;
    data_->create_time= SysDateTime::now();

    std::string filename = file;
    auto pos = filename.rfind('/');
    if (pos != std::string::npos) filename = filename.substr(pos+1);

    data_->location = "["+config.app_name+"]"
                    + "#" + to_stringf(thread_uindex, "%02d") + ":"
                    + to_hex_string((uint32_t)pthread_self()) + ":"
                    + filename + ":" + std::to_string(line) + ":"
                    + function;
  }

  virtual ~StreamLoggerHandler()
  {
    if (logging_ == false)
      return;

    // 디버그 정보를 찍을 수 있어서 삭제했다.
    // if (data_->message.tellp() == 0)
    //   return;
    if (writer_.push(data_) != 0)
      std::cout << data_->to_json() << std::endl;
  }

  StreamLoggerHandler &
  operator<<(const Delim &delim)
  {
    if (logging_ == false)
      return *this;

    delim_ = delim.delim;
    return *this;
  }

  StreamLoggerHandler &
  set_type(const int &type)
  {
    if (logging_ == false)
      return *this;

    data_->type = type;
    return *this;
  }

  template<typename T> StreamLoggerHandler &
  operator<<(const T &mesg)
  {
    if (logging_ == false)
      return *this;

    if (data_->message.tellp() > 0)
      data_->message << delim_;

    data_->message << mesg;
    return *this;
  }

  StreamLoggerHandler &
  operator<<(const bool &mesg)
  {
    if (logging_ == false)
      return *this;

    if (data_->message.tellp() > 0)
      data_->message << delim_;

    mesg == true ? data_->message << "true" : data_->message << "false";
    return *this;
  }

  StreamLoggerHandler &
  operator<<(std::ostream& (*rhs)(std::ostream&))
  {
    (void)rhs;
    return *this;
  }

private:
  StreamLoggerConfig config_;
  StreamLoggerWriter &writer_;
  std::shared_ptr<StreamLoggerData> data_;

private:
  bool logging_ = true;
  std::string delim_ = " ";
};

template<> inline StreamLoggerHandler &
StreamLoggerHandler::operator<<(const int8_t &mesg)
{
  if (logging_ == false)
    return *this;

  if (data_->message.tellp() > 0)
    data_->message << delim_;

  data_->message << (int32_t)mesg;
  return *this;
}

template<> inline StreamLoggerHandler &
StreamLoggerHandler::operator<<(const uint8_t &mesg)
{
  if (logging_ == false)
    return *this;

  if (data_->message.tellp() > 0)
    data_->message << delim_;

  data_->message << (uint32_t)mesg;
  return *this;
}

