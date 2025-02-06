#include "setup.h"

#include "TrapInfoList.h"

#include "NatsSenders.h"
#include "AuthFilterRecvers.h"

#include <extra/StopWaiter.h>
#include <csignal>
#include <unistd.h>

int main(int argc, char** argv)
{
  signal(SIGPIPE, SIG_IGN); // TCP 소켓 통신에서 상대방이 연결을 끊었는데 데이터를 보내려고 하면 SIGPIPE가 발생

  if (setup_config (argv, argc) == false) return -1;
  if (setup_cnapsdb()           == false) return -1;

//  filter_logger_user_log_func = [](const StreamLoggerData &data)
//  {
//    std::ofstream file(app_conf.procname + ".log", std::ios::app);
//    if (file.is_open() == false)
//      return true;
//
//    file << data.to_json() << "\n";
//    return true;
//  };

  // 기동 순서
  // sender, result, recver
  nats_sender.set_publisher_num     (app_conf.nats_sender_num.load())           /// next filter(송신부) thread num
             .set_queue_size        (app_conf.nats_sender_queue.load())         /// queue size(lockfree queue)
             .set_server_urls       (app_conf.nats_sender_urls.load())          /// nats server urls
             .set_subject           (app_conf.nats_sender_subject.load());      /// interest subject

  nats_result.set_publisher_num     (app_conf.nats_result_num.load())           /// result_proc(송신부) thread num
             .set_queue_size        (app_conf.nats_result_queue.load())         /// queue size(lockfree queue)
             .set_server_urls       (app_conf.nats_result_urls.load())          /// nats server urls
             .set_subject           (app_conf.nats_result_subject.load());      /// publish subject

  AuthFilterRecvers nats_recver;
  nats_recver.set_client_num        (app_conf.nats_recver_num.load())           /// nats client(수신부) thread num
             .set_server_urls       (app_conf.nats_recver_urls.load())          /// nats server urls
             .set_subject           (app_conf.nats_recver_subject.load())       /// interest subject
             .set_queue_group_name  (app_conf.nats_recver_group.load())         /// nats queue group name
             .set_worker_num        (app_conf.nats_recver_worker_num.load())    /// worker thread num
             .set_worker_queue_size (app_conf.nats_recver_worker_queue.load()); /// worker queue size(lockfree queue)

  SCOPE_EXIT(
  {
    /// 종료시 거꾸로. 로거는 끝까지 남아야.
    /// 모든 객체들은 stop을 호출하면 수신을 멈추고(unsubscribe)
    /// 수신전까지 가지고 있는 데이터를 처리 후 종료 후 종료됩니다.
    nats_recver   .stop();
    nats_result   .stop();
    nats_sender   .stop();
    trap_info_list.stop();
    cnaps_db      .stop();
    Logger::       stop();
    /// 로거(실제로 로거 쓰레드)가 중지되어도 출력을 지원합니다.
    ap_log.info() << "Stop" << app_conf.procname;
  });

  if (trap_info_list.start() == false) return -1;
  if (nats_sender   .start() == false) return -1;
  if (nats_result   .start() == false) return -1;
  if (nats_recver   .start() == false) return -1;

  ap_log.info() << "Start" << app_conf.procname;

  StopWaiter waiter;
  lambda_signal_handler<SIGINT >([&]() { waiter.stop(); });
  lambda_signal_handler<SIGTERM>([&]() { waiter.stop(); });

  waiter.wait();

  return 0;
}

