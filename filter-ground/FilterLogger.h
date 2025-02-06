/*
 * FilterLogger.h
 *
 *  Created on: 2025. 1. 26.
 *      Author: tys
 */

#pragma once

#include <stream_logger/StreamLogger.h>

/**
 * 특징
 *  - 스트림 형식의 로그 작성(std::cout 스타일)
 *    > 다양한 형식의 데이터가 컴파일 타임에 결정되어 컴파일 되므로 타입안정성이 보장됩니다.
 *  - boost::lockfree::queue 로그 쓰레드를 사용합니다.
 *  - SRP원칙에 따라 5개의 클래스로 분리하여 작성되었습니다.
 *
 * // 아래와 같은 형식의 로그를 출력합니다.
{
 "logType": "application",
 "logLevel": "info",
 "createTime": "2025-01-29 00:46:09.979",
 "logData": {
  "location": "[sensing-filter]#12:0xA27FC640:FilterWorker.cpp:63:to_next_nats",
  "message": "out tps: 1 :next nats: /sfs/result_proc {\"messageInfo\":{\"interfaceFd\":0,\"interfaceSystemId\":0,\"messageType\":1,\"detailType\":0,\"messageKey\":\"\",\"mfsgwFd\":0,\"mfsgwId\":0,\"mfsgwSessionId\":0,\"sequenceNumber\":0,\"smsServiceId\":\"\",\"messageId\":\"\",\"originationMdn\":\"\",\"destinationMdn\":\"0100010002\",\"callbackMdn\":\"\",\"messageOrigination\":3,\"mediaContent\":[],\"cnnScore\":0.0,\"anomalCount\":0,\"traceType\":0,\"url\":[\"url1\",\"url2\"]},\"customerInfo\":{\"filterFlag\":0,\"addFlag\":0,\"traceFlag\":0,\"impersonateAgree\":0,\"fsecAgree\":0,\"kisaFlag\":0,\"deepmsgFlag\":0},\"resultInfo\":{\"smppResult\":0,\"resultCode\":0,\"reasonCode\":0,\"spamPattern1\":\"\",\"filterStartTime\":1738079168978,\"filterEndTime\":1738079169978,\"filteringTime\":[{\"filterName\":\"sensing-filter\",\"startTime\":1738079169978,\"endTime\":1738079169979}]}}"
 }
}
 *  location: [app_name]#app에서 할당한 thread id:pthread_id:filename:line:function
 *  message: 출력할 메시지
 *
 * // 로그 설정
 * filter_logger_config_app_name = "app_name";
 * filter_logger_config_info     = true;
 * filter_logger_config_warn     = true;
 * filter_logger_config_error    = true;
 * filter_logger_config_debug    = true;
 * filter_logger_config_pretty   = true; // json pretty 출력
 *
 * // 쓰레드를 시작하지 않더라도 표준출력으로 로그를 출력합니다.
 * // 로그 쓰레드 시작전 필요한 사항들을 출력할 수 있게 하기 위함입니다.
 * filter_logger_thread_start;
 */



/**
 * 사용자 정의 로그 출력 함수
 * 사용자 정의 로그 함수가 정의되어 있다면 출력 직전에 호출하여 줍니다.
 * 이 함수는 로거 출력 쓰레드에서 호출됩니다.
 * 참조캡쳐를 사용한다면 캡쳐되는 변수의 수명에 주의하세요.
 *
 * std::function<bool(const StreamLoggerData &data)> 형식으로
 * return이 true이면 표준출력 아니면 표준출력은 하지 않습니다.

 * logger_user_log_func = [](const StreamLoggerData &data) -> bool
 * { cout << data.to_json() << endl; return true; };
 *  *
 * // application type, info 로그를 출력합니다.
 * ap_log.info() << "hello" << "world" << 123 << 3.14;
 *
 * // transaction type, debug 로그를 출력합니다.
 * tr_log.debug() << "hello" << "world" << 123 << 3.14;
 *
 * // sensing type, error 로그를 출력합니다.
 * ss_log.error() << "hello" << "world" << 123 << 3.14;
 *
 * // 로그 쓰레드를 정지시킵니다.
 * logger_stop;
 *
 */



/**
 * 사용자 delimeter정의
 *
 * delimeter를 사용하지 않으면 기본값은 " "입니다.
 * ap_log.info() << "hello" << "world" << 123 << 3.14;
 * -> hello world 123 3.14
 *
 * ap_log.info() << Delim("|") << "hello" << "world" << 123 << 3.14;
 * -> hello|world|123|3.14
 *
 * ap_log.info() <<"hello" << "world" << 123 << Delim("-") << 3.14;
 * -> hello world 123-3.14
 */

// 축약형
#define ap_log  StreamLogger(__FILE__, __LINE__, __FUNCTION__, filter_logger_config, filter_logger_writer).ap()
#define tr_log  StreamLogger(__FILE__, __LINE__, __FUNCTION__, filter_logger_config, filter_logger_writer).tr()
#define ss_log  StreamLogger(__FILE__, __LINE__, __FUNCTION__, filter_logger_config, filter_logger_writer).ss()

// 이전 인터페이스 호환용
#define sfs_log                       ap_log
#define sfs_transaction_log           tr_log
#define sfs_sensing_log               ss_log

#define logger_delim(v)               StreamLoggerHandler::Delim(v)
#define logger_nodelim                StreamLoggerHandler::Delim()

/// 쓰레드를 시작하지 않더라도 표준출력으로 로그를 출력합니다.
#define filter_logger_writer          FilterLoggerWriter::ref()

#define filter_logger_thread_start    filter_logger_writer.start()
#define filter_logger_thread_stop     filter_logger_writer.stop()

#define filter_logger_config          FilterLoggerConfig::ref()
#define filter_logger_config_app_name filter_logger_config.app_name
#define filter_logger_config_info     filter_logger_config.level.info  // = true, false
#define filter_logger_config_warn     filter_logger_config.level.warn  // = true, false
#define filter_logger_config_error    filter_logger_config.level.error // = true, false
#define filter_logger_config_debug    filter_logger_config.level.debug // = true, false

/// 로그를 출력할 때 json을 이쁘니 모드로 출력할지 여부, false이면 한줄로 출력합니다.
#define filter_logger_config_pretty   filter_logger_config.pretty      // = true, false

/// 사용자 정의 로그 출력 함수
/// std::function<bool(const StreamLoggerData &data)> 형식으로
/// return이 true이면 표준출력 아니면 표준출력은 하지 않습니다.
/// * 사용자 정의 로그 함수가 정의되어 있다면 출력 직전에 호출하여 줍니다.
/// * 이 함수는 로거 출력 쓰레드에서 호출됩니다.
/// * 참조캡쳐를 사용한다면 캡쳐되는 변수의 수명에 주의하세요.
#define filter_logger_user_log_func   filter_logger_writer.user_log_func

/// 필터 서비스 만으로 특화시키기
class FilterLoggerConfig : public StreamLoggerConfig, public Singleton<FilterLoggerConfig> {};
class FilterLoggerWriter : public StreamLoggerWriter, public Singleton<FilterLoggerWriter> {};

#define filter_logger_debug_on        filter_logger_config.level.debug
















