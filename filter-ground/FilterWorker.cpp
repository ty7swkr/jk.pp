#include "FilterWorker.h"

#include <Logger.h>
#include <extra/ScopeExit.h>
#include <extra/Optional.h>
#include <extra/SysDateTimeDiff.h>
#include <extra/ThreadUniqueIndexer.h>

void
FilterWorker::run()
{
  queueable_t<std::tuple<std::string, filter_info_t, SysDateTime>> queueable_item; // = 0;

  sfs_log.info() << "Start FilterWorker:" << assigned_no_str();

  // 0 : 정상수신
  // -1 : 큐 닫힘
  while (waiter_.pop(queueable_item) == 0)
  {
    handle_discard_ = false;
    auto item = queueable_item.take();

    auto tuple      = *(item.get());
    auto &subject   = std::get<0>(tuple);
    auto &filter    = std::get<1>(tuple);
    auto &recv_time = std::get<2>(tuple);

    SCOPE_EXIT({
      if (handle_discard_ == true) return;
      tps_meter_out.add_transaction();
    });

    handle_filter(filter, subject, recv_time);
  } // end of while

  sfs_log.info() << "Stop FilterWorker:" << assigned_no_str();
}

void
FilterWorker::set_filtering_time(filtering_time_objs &filtering_times, const SysDateTime &recv_time) const
{
  auto &filtering_time = filtering_times[get_app_conf().procname];
  filtering_time.filterName = get_app_conf().procname;
  filtering_time.startTime  = recv_time.duration().millisecs();
  filtering_time.endTime    = SysDateTime::now().duration().millisecs();
}

void
FilterWorker::to_result_nats(filter_info_t &filter,  const SysDateTime &recv_time,
                             const int &smpp_result, const int &result_code, const int &reason_code) const
{
  result_info_t &result = filter.resultInfo;
  result.smppResult = smpp_result;
  result.resultCode = result_code;
  result.reasonCode = reason_code;

  set_filtering_time(result.filteringTime, recv_time);
  sfs_log.info() << "out tps:" << (handle_discard_ ? tps_meter_out.get_tps() : tps_meter_out.get_tps()+1) << ":result nats:"
                 << (filter_logger_debug_on ? get_app_conf().nats_sender_subject.load()+" "+to_json(filter) : get_app_conf().nats_sender_subject.load());
  nats_result.publish(to_json(filter));
}

void
FilterWorker::to_next_nats(filter_info_t &filter, const SysDateTime &recv_time) const
{
  set_filtering_time(filter.resultInfo.filteringTime, recv_time);
  sfs_log.info() << "out tps:" << tps_meter_out.get_tps()+1 << ":next nats:"
                 << (filter_logger_debug_on ? get_app_conf().nats_sender_subject.load()+" "+to_json(filter) : get_app_conf().nats_sender_subject.load());
  nats_sender.publish(to_json(filter));
}

Optional<filter_info_t>
FilterWorker::parse_message(const std::string &message) const
{
  auto filter = from_filter_info_json(message);
  if (filter == false)
  {
    error_toggle_.turn_on();
    sfs_log.error() << filter.error();
    return nullopt;
  }

  if (error_toggle_.turn_off() == true)
    sfs_log.info() << "JSON parse error cleared.";

  return filter.value();
}

int
FilterWorker::push(const std::pair<std::string, std::string> &subject_message)
{
  SysDateTime recv_time = SysDateTime::now();

  auto filter = parse_message(subject_message.second);
  if (filter.has_value() == false)
    return 0;

  {
    SCOPE_EXIT(
    { sfs_log.info() << "in tps:" << tps_meter_in.get_tps()
                     << ":recv nats:"
                     << (filter_logger_debug_on ? subject_message.first+" "+subject_message.second : subject_message.first); });

    if (discard_tps_in(filter.value(), recv_time) == true)
      return 0;

    tps_meter_in.add_transaction();
  }

  auto res = try_push({subject_message.first, filter.value(), recv_time}, 1000); ///< 1000 : max_retries
  if (res < 0)
    return res;

  if (res == EAGAIN)
  {
    discard_queue_full(filter.value(), recv_time);
    return 0;
  }

  return res;
}

bool
FilterWorker::handle_discard(filter_info_t &filter, const SysDateTime &recv_time) const
{
  ScopeExit rvalue([&](){handle_discard_ = true;});

  if (discard_tps_out(filter, recv_time) == true)
    return true;

  if (discard_timeout(filter, recv_time) == true)
    return true;

  rvalue.ignore();
  return false;
}

//  제어는 HAM 처리
bool
FilterWorker::discard_tps_in(filter_info_t &filter, const SysDateTime &recv_time) const
{
  auto current = tps_meter_in.get_tps();
  if (current < get_app_conf().discard_tps_in.load())
    return false;

  filter.resultInfo.spamPattern1 = std::to_string(current) + "/" + std::to_string(get_app_conf().discard_tps_in.load());
  to_result_nats(filter, recv_time, SMPP_DISCARD, TRANS_RESULT_CODE_HAM_FAIL, DISCARD_ENQUEUE_TPS);
  return true;
}

//  제어는 HAM 처리
bool
FilterWorker::discard_tps_out(filter_info_t &filter, const SysDateTime &recv_time) const
{
  auto current = tps_meter_out.get_tps();
  if (current < get_app_conf().discard_tps_out.load())
    return false;

  filter.resultInfo.spamPattern1 = std::to_string(current) + "/" + std::to_string(get_app_conf().discard_tps_out.load());
  to_result_nats(filter, recv_time, SMPP_DISCARD, TRANS_RESULT_CODE_HAM_FAIL, DISCARD_DEQUEUE_TPS);
  return true;
}

bool
FilterWorker::discard_timeout(filter_info_t &filter, const SysDateTime &recv_time) const
{
  // gwrecv 수신시간부터 지금까지.
  int64_t duration_millisecs = SysDateTime::now().duration().millisecs() - filter.resultInfo.filterStartTime;
  if (duration_millisecs < get_app_conf().discard_timeout_ms.load())
    return false;

  filter.resultInfo.spamPattern1 = std::to_string(duration_millisecs) + "/" + std::to_string(get_app_conf().discard_timeout_ms.load());
  to_result_nats(filter, recv_time, SMPP_DISCARD, TRANS_RESULT_CODE_HAM_FAIL, DISCARD_TIMEOUT);
  return true;
}

bool
FilterWorker::discard_queue_full(filter_info_t &filter, const SysDateTime &recv_time) const
{
  filter.resultInfo.spamPattern1 = std::to_string(waiter_.size()) + "/" + std::to_string(get_app_conf().discard_queue_size.load());
  to_result_nats(filter, recv_time, SMPP_DISCARD, TRANS_RESULT_CODE_HAM_FAIL, DISCARD_QUEUEFULL);
  return true;
}

