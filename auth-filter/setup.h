/*
 * setup.h
 *
 *  Created on: 2024. 11. 27.
 *      Author: tys
 */

#pragma once

#include "AuthFilterConf.h"
#include <Logger.h>
#include <CnapsDB.h>
#include <extra/ScopeExit.h>

inline bool
setup_config(char **argv, int argc)
{
  (void)argc;
  app_conf.procname = argv[0];

  auto pos = app_conf.procname.rfind('/');
  if (pos != std::string::npos)
    app_conf.procname = app_conf.procname.substr(pos+1);

  std::string filename = "app.conf";
  if (std::getenv("CNAPS_DEV_LOCAL") != nullptr)
    filename = "dev.conf";

  if (app_conf.read(filename) == false)
    return false;

  Logger::start(app_conf.procname,
                app_conf.log_error.load(),
                app_conf.log_warn .load(),
                app_conf.log_info .load(),
                app_conf.log_debug.load());
  return true;
}

inline bool
setup_cnapsdb()
{
  auto db_config = app_conf.get_next_db_config();

  cnaps_db.set_connection_info(db_config.url, db_config.properties());

  cnaps_db.reset_connection_info = [&]()
  {
    auto reloaded_db_config = app_conf.get_next_db_config(cnaps_db.url());

    if (reloaded_db_config.url          == cnaps_db.url() &&
        reloaded_db_config.properties() == cnaps_db.properties())
      return;

    cnaps_db.set_connection_info(reloaded_db_config.url,
                                 reloaded_db_config.properties());
  };

  // 커넥션을 테스트 해본다.
  if (cnaps_db.test_connection([&](sql::SQLException &e)
  {
    ap_log.error() << db_config.url;
    ap_log.error() << db_config.user << db_config.password;
    ap_log.error() << e.getErrorCode() << ":" << e.what() << ":" << e.getSQLState();
  }) == false)
    return false;

  // 코랙 함수들 등록
  // 실패시 호출되는 함수.
  cnaps_db.occur_connect_error = [&](sql::SQLException &e)
  {
    ap_log.error() << e.getErrorCode() << ":" << e.what() << ":" << e.getSQLState();
  };

  // 성공시 호출되는 함수.
  cnaps_db.clear_connect_error = [&]()
  {
    ap_log.info() << "clear DB Connect error";
  };

  cnaps_db.start();
  return true;
}






