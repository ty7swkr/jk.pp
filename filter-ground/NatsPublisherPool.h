/*
 * NatsSender.h
 *
 *  Created on: 2024. 12. 3.
 *      Author: tys
 */

#pragma once

#include <NatsPublisher.h>
#include <Logger.h>
#include <future>
#include <deque>

/**
 * @class NatsPublisherPool
 * @brief 여러 NATS 발행자를 관리하는 풀 클래스
 * @details Round-robin 방식으로 메시지를 분산하여 발행합니다.
 */
class NatsPublisherPool
{
public:
  /**
   * @struct params_t
   * @brief 발행자 풀의 설정 파라미터
   */
  struct params_t
  {
    size_t                    publisher_num = 1;      ///< 발행자 수
    size_t                    queue_size    = 10000;  ///< 각 발행자의 큐 크기
    std::vector<std::string>  urls;                    ///< NATS 서버 URL
    std::string               subject;                ///< 발행 주제
  };

  /**
   * @brief 발행자 수 설정
   * @param num 발행자 수 (기본값: 1)
   * @return 현재 객체 참조
   */
  NatsPublisherPool &set_publisher_num(const size_t &num = 1)
  {
    params_.publisher_num = num;
    return *this;
  }

  /**
   * @brief 큐 크기 설정
   * @param queue_size 큐 크기
   * @return 현재 객체 참조
   */
  NatsPublisherPool &set_queue_size(const size_t &queue_size)
  {
    params_.queue_size = queue_size;
    return *this;
  }

  /**
   * @brief 서버 URL 설정
   * @param url NATS 서버 URL
   * @return 현재 객체 참조
   */
  NatsPublisherPool &set_server_urls(const std::vector<std::string> &urls)
  {
    params_.urls = urls;
    return *this;
  }

  /**
   * @brief 발행 주제 설정, 사용해도 되고 안해도 됨.
   * @param subject 발행할 주제
   * @return 현재 객체 참조
   */
  NatsPublisherPool &set_subject(const std::string &subject)
  {
    params_.subject = subject;
    return *this;
  }

  /**
   * @brief 메시지 발행
   * @param message 발행할 메시지
   * @param subject subject
   */
  bool publish(const std::string &message, std::string subject = "")
  {
    if (subject.empty() == true)
    {
      if (params_.subject.empty() == true) { sfs_log.error() << "subject is empty"; return false; }
      subject = params_.subject;
    }

    publishers_[sequence_++ % publishers_.size()].publish(subject, message);
    return true;
  }

  /**
   * @brief 발행자 풀 시작
   * @return 시작 성공 여부
   */
  bool start()
  {
    this->stop();

    for (size_t index = 0; index < params_.publisher_num; ++index)
    {
      publishers_.emplace_back(params_.queue_size);
      publishers_.back().assigned_no(index+1);
    }

    for (auto &publisher : publishers_)
    {
      if (publisher.set_server_urls(params_.urls).start() == false)
      {
        this->stop();
        return false;
      }
    }

    return true;
  }

  /**
   * @brief 발행자 풀 중지
   */
  void stop()
  {
    for (auto &publisher : publishers_)
      publisher.stop();

    publishers_.clear();
  }

protected:
  params_t params_;                       ///< 설정 파라미터
  std::deque<NatsPublisher> publishers_;  ///< 발행자 목록
  size_t sequence_ = 0;                   ///< Round-robin 시퀀스
};
