/*
 * AppConf.h
 *
 *  Created on: 2024. 11. 26.
 *      Author: tys
 */

#include "AppConf.h"
#include <Logger.h>
#include <extra/helper.h>
#include <set>

bool
AppConf::read(const std::string &filename, std::function<bool(const MJsonObject &config)> user_config)
{
  auto app_conf_path_env = std::getenv("CNAPS_CONFIG_PATH");
  if (app_conf_path_env == nullptr)
  {
    static const char *kuber_err = R"(
{
 "logType": "application",  
 "logLevel": "error",
 "logData": {
 "message": "There is no CNAPS_CONFIG_PATH environment setting."
 }
})";
    std::cerr << kuber_err << std::endl;
    return false;
  }

  std::string conf_path = app_conf_path_env;

  std::string conf_file = filename;
  if (conf_path.length() > 0)
  {
    if (conf_path.back() != '/') conf_path += '/';
    conf_file = conf_path + filename;
  }

  try
  {
    // 파일에서 JSON 파싱
    MJsonObject config = parse_file(conf_file, [&](const std::string &json_conf) { config_str_ = json_conf; });

    std::vector<db_config_t> dbs;
    // database 설정을 중첩된 required()를 사용하여 파싱
    config.required("database", [&](const MJsonObject &database)
    {
      database.required_for("mariadb", [&](const MJsonArray &mariadb, size_t index)
      {
        db_config_t db_config;
        db_config.url       = mariadb[index]["url" ].as_string();
        db_config.user      = mariadb[index]["id"  ].as_string();
        db_config.password  = mariadb[index]["pwd" ].as_string();
        dbs.emplace_back(db_config);
      });
    });
    this->db_configs = dbs;

    // system 설정을 required()를 사용하여 파싱
    system_id = config["system_id"].as_uint32();

    // NATS 설정을 계층적으로 required()를 사용하여 파싱
    config.required("nats", [&](const MJsonObject &nats)
    {
      // receiver 설정 파싱
      nats.required("recv", [&](const MJsonObject &recv)
      {
        std::set<std::string> url_arr;
        recv.required_for("urls", [&](const MJsonArray &urls, size_t index) { url_arr.insert(urls[index].as_str()); });
        nats_recver_urls    = std::vector<std::string>(url_arr.begin(), url_arr.end());
        nats_recver_subject = recv["subject"].as_string();
        nats_recver_group   = recv["group"  ].as_string();
        nats_recver_num     = recv["num"    ].as_uint32();

        // worker 설정 파싱
        recv.required("worker", [&](const MJsonObject &worker)
        {
          nats_recver_worker_num  = worker["num"        ].as_uint32();
          nats_recver_worker_queue= worker["queue_size" ].as_uint32();
        });
      });

      // next filter 설정 파싱
      nats.required("next", [&](const MJsonObject &next)
      {
        std::set<std::string> url_arr;
        next.required_for("urls", [&](const MJsonArray &urls, size_t index) { url_arr.insert(urls[index].as_str()); });
        nats_sender_urls    = std::vector<std::string>(url_arr.begin(), url_arr.end());
        nats_sender_subject = next["subject"    ].as_string();
        nats_sender_num     = next["num"        ].as_uint32();
        nats_sender_queue   = next["queue_size" ].as_uint32();
      });

      // result 설정 파싱
      nats.required("result", [&](const MJsonObject &result)
      {
        std::set<std::string> url_arr;
        result.required_for("urls", [&](const MJsonArray &urls, size_t index) { url_arr.insert(urls[index].as_str()); });
        nats_result_urls    = std::vector<std::string>(url_arr.begin(), url_arr.end());
        nats_result_subject = result["subject"    ].as_string();
        nats_result_num     = result["num"        ].as_uint32();
        nats_result_queue   = result["queue_size" ].as_uint32();
      });

      // discard 설정을 required()를 사용하여 파싱
      nats.required("discard", [&](const MJsonObject &discard)
      {
        discard_timeout_ms  = discard["timeout_ms"  ].as_uint32();
        discard_queue_size  = discard["queue_size"  ].as_uint32();
        discard_tps_in      = discard["enqueue_tps" ].as_uint32();
        discard_tps_out     = discard["dequeue_tps" ].as_uint32();
      });
    });

    config.required("log_level", [&](const MJsonObject &log_level)
    {
      log_error  = log_level["ERROR"].as_bool();
      log_warn   = log_level["WARN" ].as_bool();
      log_info   = log_level["INFO" ].as_bool();
      log_debug  = log_level["DEBUG"].as_bool();
    });

    // 사용자 정의 설정 처리 콜백이 있는 경우 실행
    if (user_config != nullptr)
      return user_config(config);

    return true;
  }
  catch (const std::runtime_error& e)
  {
    sfs_log.error() << e.what();
    return false;
  }
}

AppConf::db_config_t
AppConf::get_next_db_config(const std::string &curr_url) const
{
  auto configs = db_configs.load();
  if (configs.size() == 0)
    return db_config_t();

  // curr_config가 마지막 요소인 경우 첫 번째 요소 반환
  if (curr_url == configs.back().url)
    return configs.front();

  // curr_config의 다음 요소를 찾아 반환
  for (size_t index = 0; index < configs.size() - 1; ++index)
    if (configs[index].url == curr_url)
      return configs[index + 1];

  // curr_config가 vector에 없는 경우 첫 번째 요소 반환
  return configs.front();
}



