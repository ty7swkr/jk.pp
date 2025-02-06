/*
 * AuthFilterWorker.h
 *
 *  Created on: 2024. 12. 2.
 *      or: tys
 */

#pragma once

#include <FilterWorker.h>
#include "AuthFilterConf.h"

class AuthFilterWorker : public FilterWorker ///< filter_01_t, 수신시간
{
public:
  AuthFilterWorker(const size_t &queue_size = 10000)
  : FilterWorker(queue_size) {}

protected:
  bool set_customer_info(filter_info_t &filter, const SysDateTime &recv_time);
  void handle_filter    (filter_info_t &filter, const std::string &subject, const SysDateTime &recv_time) override;

protected:
  const AppConf &get_app_conf() const override { return app_conf; }
};
