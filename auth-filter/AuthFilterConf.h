/*
 * AuthFilterConf.h
 *
 *  Created on: 2024. 11. 26.
 *      Author: tys
 */

#pragma once

#include <AppConf.h>

#define app_conf AuthFilterConf::ref()

class AuthFilterConf : public AppConf, public Singleton<AuthFilterConf>
{
public:
   // SmishingUrl 테이블 감시 주기.
  std::atomic<uint32_t> table_check_period_ms{1000};

  // functions
  bool read(const std::string &filename)
  {
    return AppConf::read(filename, [&](const MJsonObject &config)
    {
      try
      {
        config["database"]["table_check_period_ms"].as_int();
      }
      catch (const std::runtime_error &e)
      {
        ap_log.error() << e.what();
        return false;
      }
      return true;
    });
  }
};




