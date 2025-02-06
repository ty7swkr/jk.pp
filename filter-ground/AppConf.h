/*
 * AppConf.h
 *
 *  Created on: 2024. 11. 26.
 *      Author: tys
 */

#pragma once

#include <Logger.h>
#include <mariadb/conncpp.hpp>
#include <extra/MJsonObject.h>
#include <extra/Singleton.h>
#include <extra/LockedObject.h>
#include <extra/helper.h>
#include <cstdlib>

/***
 * @brief 애플리케이션 설정 정보 클래스
 * @details 애플리케이션 설정 정보를 관리하는 클래스
 */
class AppConf
{
public:
  AppConf()
  {
    // default value
//    std::map<std::string, stream_logger::log_type_entry_t> default_log_types;
//    default_log_types["PROCESS"].bUse       = false;
//    default_log_types["PROCESS"].filename   = "process.log";
//    default_log_types["PROCESS"].max_size   = 1024*1024*1024;
//    default_log_types["PROCESS"].queue_size = 1000;
//    log_types = default_log_types;
  }

  virtual ~AppConf() {}
  struct db_config_t
  {
    std::string url;
    std::string user;
    std::string password;

    sql::Properties
    properties()
    {
      return {{"user", user}, {"password", password}};
    }
  };

  // configuration
  std::string               procname;
  std::string               hostname;
  std::atomic<uint32_t>     system_id{43};
  LockedObject<std::vector<db_config_t>> db_configs;

  LockedObject<std::vector<std::string>> nats_recver_urls;
  LockedObject<std::string> nats_recver_subject;
  LockedObject<std::string> nats_recver_group;
  std::atomic<uint32_t>     nats_recver_num;
  std::atomic<uint32_t>     nats_recver_worker_num;
  std::atomic<uint32_t>     nats_recver_worker_queue;

  LockedObject<std::vector<std::string>> nats_sender_urls;
  LockedObject<std::string> nats_sender_subject;
  std::atomic<uint32_t>     nats_sender_num;
  std::atomic<uint32_t>     nats_sender_queue;

  LockedObject<std::vector<std::string>> nats_result_urls;
  LockedObject<std::string> nats_result_subject;
  std::atomic<uint32_t>     nats_result_num;
  std::atomic<uint32_t>     nats_result_queue;

  std::atomic<uint32_t>     discard_timeout_ms{3000};
  std::atomic<uint32_t>     discard_queue_size{1000};
  std::atomic<uint32_t>     discard_tps_in    {1000};
  std::atomic<uint32_t>     discard_tps_out   {1000};

  std::atomic<bool> log_error{true};
  std::atomic<bool> log_warn {true};
  std::atomic<bool> log_info {true};
  std::atomic<bool> log_debug{true};
//  LockedObject<std::map<std::string, stream_logger::log_type_entry_t>> log_types;

  /***
   * @brief 설정 파일을 읽어서 설정 정보를 파싱
   * @param file_name 설정 파일 이름
   * @param user_config 사용자 설정 콜백 함수
   * @return 설정 정보 파싱 성공 여부
   */
  virtual bool read(const std::string &filename,
                    std::function<bool(const MJsonObject &config)> user_config = nullptr);

  // curr_url이 공백이면 맨 처음걸 적용.
  db_config_t get_next_db_config(const std::string &curr_url = "") const;
  std::string to_string         () const { return config_str_.load(); }

protected:
  LockedObject<std::string> config_str_;
};


