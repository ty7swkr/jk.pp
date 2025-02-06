/*
 * StreamLogger.h
 *
 *  Created on: 2024. 12. 13.
 *      Author: tys
 */

#pragma once

#include <FilterLogger.h>
#include <extra/MJsonObject.h>
#include <extra/ScopeExit.h>
#include <asis_predefined.h>

class Logger
{
public:
  static void stop()
  {
    filter_logger_thread_stop;
  }

  static bool start(const std::string &procname,
                    const bool &err   = true,
                    const bool &warn  = true,
                    const bool &info  = true,
                    const bool &debug = true)
  {
    filter_logger_config_app_name= procname;
    filter_logger_config_pretty  = true;
    filter_logger_config_error   = err;
    filter_logger_config_warn    = warn;
    filter_logger_config_info    = info;
    filter_logger_config_debug   = debug;
    filter_logger_thread_start;

    return true;
  }
};
