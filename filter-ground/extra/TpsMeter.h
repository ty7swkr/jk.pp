/*
 * TpsMeter.h
 *
 *  Created on: 2024. 11. 27.
 *      Author: tys
 */

#pragma once

#include <chrono>
#include <set>
#include <mutex>

class TpsMeter
{
public:
  // return tps
  size_t add_transaction()
  {
    std::lock_guard<std::mutex> guard(mutex_);
    transactions_.insert(std::chrono::system_clock::now());
    return get_tps_nolock();
  }

  size_t get_tps()
  {
    std::lock_guard<std::mutex> guard(mutex_);
    return get_tps_nolock();
  }

protected:
  size_t get_tps_nolock()
  {
    cleanup_cutoff();
    return transactions_.size();
  }

  void cleanup_cutoff()
  {
    auto cutoff = std::chrono::system_clock::now() - std::chrono::seconds(1);
    transactions_.erase(transactions_.lower_bound(cutoff), transactions_.end());
  }

protected:
  mutable std::mutex mutex_;
  std::multiset<std::chrono::system_clock::time_point,
                std::greater<std::chrono::system_clock::time_point>> transactions_;
};

