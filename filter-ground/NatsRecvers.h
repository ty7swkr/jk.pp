/*
 * NatsRecvers.h
 *
 *  Created on: 2024. 12. 2.
 *      Author: tys
 */

#pragma once

#include <Logger.h>
#include <FilterTpsMeter.h>
#include <extra/ScopeExit.h>
#include <extra/Toggle.h>

#include <sfs_nats_cli.h>
#include <deque>
#include <future>

using NatsClient = SfsNatsClient<std::string>;

// NatsRecvers
// +---------------+    +-----------------+
// |  NATS Client  | -> |  Worker Pool    |
// +---------------+    |                 |
//                      |  Worker 1       |
//                      |  Worker 2       |
//                      |  Worker 3       |
//                      |  Worker n...    |
//                      +-----------------+

/**
 * @class NatsRecvers
 * @brief NATS 메시지 수신 및 워커 풀 관리 클래스
 * @tparam WORKER_POOL 작업을 처리할 워커 풀 타입
 * @details
 * NATS 서버로부터 메시지를 수신하여 워커 풀로 전달하는 관리자 클래스입니다.
 * 여러 NATS 클라이언트와 워커 풀을 관리하며, 수신된 메시지를 워커들에게 분배합니다.
 */
template<typename WORKER_POOL>
class NatsRecvers
{
public:
  /**
   * @struct params_t
   * @brief 수신자 설정 파라미터
   */
  struct params_t
  {
    size_t                    client_num = 1;           ///< 클라이언트 수
    size_t                    worker_num = 1;           ///< nats 클라이언트당 워커 수
    size_t                    worker_queue_size = 10000;///< 워커 큐 크기
    std::vector<std::string>  urls;                     ///< 서버 URL
    std::string               subject;                  ///< 구독 주제
    std::string               queue_group;              ///< 큐 그룹명
    std::deque<std::pair<std::string, std::string>> subjects; ///< 주제, 그룹
  };

  /**
   * @brief NATS 클라이언트 수 설정
   * @param num 설정할 클라이언트 수 (기본값: 1)
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   * @details 메시지 수신을 위한 NATS 클라이언트의 수를 설정합니다.
   */
  NatsRecvers &set_client_num(const size_t &num = 1)
  {
    params_.client_num = num;
    return *this;
  }

  /**
   * @brief 워커 수 설정
   * @param num 설정할 워커 수 (기본값: 1)
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   * @details 각 워커 풀에서 운영할 워커의 수를 설정합니다.
   */
  NatsRecvers &set_worker_num(const size_t &num = 1)
  {
    params_.worker_num = num;
    return *this;
  }

  /**
   * @brief 워커 큐 크기 설정
   * @param size 설정할 큐 크기 (기본값: 1)
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   * @details 각 워커가 가질 작업 큐의 크기를 설정합니다.
   */
  NatsRecvers &set_worker_queue_size(const size_t &size = 1)
  {
    params_.worker_queue_size = size;
    return *this;
  }

  /**
   * @brief NATS 서버 URL 설정
   * @param url NATS 서버 URL
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   * @details NATS 서버 연결에 사용할 URL을 설정합니다.
   */
  NatsRecvers &set_server_urls(const std::vector<std::string> &urls)
  {
    params_.urls = urls;
    return *this;
  }

  /**
   * @brief 구독 주제 설정, 사용해도 되고 안해도 됨.
   * @param subject 구독할 NATS 주제
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   * @details NATS 서버에서 구독할 메시지 주제를 설정합니다.
   */
  NatsRecvers &set_subject(const std::string &subject)
  {
    params_.subject = subject;
    return *this;
  }

  /**
   * @brief 큐 그룹명 설정, 사용해도 되고 안해도 됨.
   * @param queue_group_name 설정할 큐 그룹명
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   * @details NATS 큐 그룹 구독에 사용할 그룹명을 설정합니다.
   */
  NatsRecvers &set_queue_group_name(const std::string &queue_group_name)
  {
    params_.queue_group = queue_group_name;
    return *this;
  }

  NatsRecvers &add_subject_queue_group(const std::string &subject, const std::string &queue_group)
  {
    params_.subjects.emplace_back(subject, queue_group);
    return *this;
  }

  /**
   * @brief 수신자 시작
   * @return 시작 성공 여부
   * @details
   * - 설정된 수만큼의 클라이언트와 워커 풀을 초기화합니다.
   * - NATS 서버에 연결하고 메시지 구독을 시작합니다.
   * - 워커 풀을 시작하여 메시지 처리 준비를 완료합니다.
   */
  bool start();

  /**
   * @brief 수신자 중지
   * @details
   * - 모든 NATS 클라이언트의 연결을 종료합니다.
   * - 모든 워커 풀의 작업을 중지시킵니다.
   */
  void stop()
  {
    // 나머지 정리
    for (auto &pair : clients_)
    {
      auto &client      = pair.first;
      auto &worker_pool = pair.second;

      client->drain();
      worker_pool.stop();

      client.reset();
    }

    clients_.clear();
  }

protected:
  /**
   * @brief NATS 에러 발생시 호출되는 콜백 함수
   * @param nc NATS 연결 객체
   * @param sub NATS 구독 객체
   * @param err 발생한 에러 상태
   * @param closure 사용자 정의 데이터
   * @details NATS 서버 연결이나 구독 중 발생하는 에러를 처리합니다.
   */
  static void
  nats_error_callback(natsConnection *nc, natsSubscription *sub, natsStatus err, void *closure)
  {
    (void)nc; (void)sub; (void)err; (void)closure;
  }

protected:
  using NatsClientSptr = std::shared_ptr<NatsClient>;  ///< NATS 클라이언트 스마트 포인터 타입

  params_t params_;  ///< 수신자 설정 파라미터
  std::deque<std::pair<NatsClientSptr, WORKER_POOL>> clients_;  ///< NATS 클라이언트와 워커 풀 쌍의 목록
};

/**
 * @brief start() 함수의 구현부
 * @details
 * 1. 설정된 클라이언트 수만큼 clients_ 컨테이너를 초기화합니다.
 * 2. 각 클라이언트-워커풀 쌍에 대해:
 *    - 워커 풀을 설정된 워커 수와 큐 크기로 초기화
 *    - NATS 클라이언트 생성 및 서버 연결
 *    - 워커 풀 시작
 *    - 메시지 구독 설정
 * 3. 에러 발생시 적절한 에러 처리 수행
 */
template<typename WORKER_POOL> bool
NatsRecvers<WORKER_POOL>::start()
{
  Toggle error_toggle(false, false);

  clients_.resize(params_.client_num);

  try
  {
    for (std::pair<NatsClientSptr, WORKER_POOL> &pair : clients_)
    {
      auto &client     = pair.first;
      auto &worker_pool= pair.second;

      client = std::make_shared<NatsClient>();

      worker_pool.set_num_of_workers(params_.worker_num, params_.worker_queue_size);
      worker_pool.start();

      client->connectServers(params_.urls, NatsRecvers<WORKER_POOL>::nats_error_callback);

      if (params_.subject.empty() == false)
        params_.subjects.push_front({params_.subject, params_.queue_group});

      for (auto &pair : params_.subjects)
      {
        auto &subject = pair.first;
        auto &group   = pair.second;
        // queue group name
        client->subscribeGroup(subject, group, [&worker_pool, subject](const std::string &message)
        {
          while (true)
          {
            int res = worker_pool.push(std::make_pair(subject, message));
            switch (res)
            {
              case      0 : return; // 정상
              case     -1 : return; // stop 시그널 받을때.
              case EAGAIN :         // 큐가 꽉 찾을때. 재시도.
              default     : continue;
            }
          }
        });
      }
    }
  }
  catch (const std::bad_alloc &e)
  {
    if (error_toggle.turn_on() == true)
      sfs_log.error() << params_.subject + ": " + e.what();
    return false;
  }
  catch (const SfsNatsException &e)
  {
    if (error_toggle.turn_on() == true)
      sfs_log.error() << params_.subject + ": " + e.what();
    return false;
  }
  catch (const std::exception &e)
  {
    if (error_toggle.turn_on() == true)
      sfs_log.error() << params_.subject + ": " + e.what();
    return false;
  }

  if (error_toggle.turn_off() == true)
    sfs_log.info() << params_.subject + ": NATS reception error cleared.";

  return true;
}


