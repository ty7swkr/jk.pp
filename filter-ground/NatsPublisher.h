/*
 * NatsClientThread.h
 *
 *  Created on: 2024. 12. 3.
 *      Author: tys
 */

#pragma once

#include <queueable.h>

#include <extra/LockedObject.h>
#include <extra/LockFreeQueueThread.h>
#include <sfs_nats_cli.h>

using NatsClient = SfsNatsClient<std::string>;

/**
 * @class NatsPublisher
 * @brief NATS 서버로 메시지를 발행하는 스레드 안전한 발행자 클래스
 *
 * @details LockFreeQueueThread를 상속받아 스레드 안전한 메시지 큐잉 기능을 제공합니다.
 */
using queueable_pair = queueable_t<std::pair<std::string, std::string>>; ///< subject, message

class NatsPublisher : public LockFreeQueueThread<false, queueable_pair, boost::lockfree::fixed_sized<true>>
{
public:
  /**
   * @brief 생성자
   * @param queue_size 메시지 큐의 크기 (기본값: 10000)
   */
  NatsPublisher(const size_t &queue_size = 10000, const size_t &assigned_no = 0)
  : LockFreeQueueThread(queue_size), assigned_no_(assigned_no) {}

  virtual ~NatsPublisher() {}

  /**
   * @brief NATS 서버 URL 설정
   * @param url NATS 서버 URL
   * @return 현재 객체에 대한 참조 (메서드 체이닝용)
   */
  NatsPublisher &set_server_urls(const std::vector<std::string> &urls)
  {
    urls_ = urls;
    return *this;
  }

  NatsPublisher &assigned_no(const size_t &no)
  {
    assigned_no_ = no;
    return *this;
  }


  /**
   * @brief 발행자 시작
   * @return 시작 성공 여부
   */
  bool start() override;
  bool stop () override
  {
    /// 종료이므로 따로 drain이 필요 없다.
    /// client->flush()와 client_.reset();는 run함수 종료시로 옮겼다.
    return LockFreeQueueThread::stop();
  }

  void publish(const std::string &subject, const std::string &message)
  {
    queueable_pair item({subject, message});
    if (waiter_.push(item) == 0)
      return;
    item.destroy();
  }

protected:
  /**
   * @brief 스레드 실행 함수
   */
  void run() override;

protected:
  size_t assigned_no_ = 0;
  LockedObject<std::vector<std::string>> urls_;      ///< 서버 URL
  std::unique_ptr<NatsClient> client_; ///< NATS 클라이언트
};
