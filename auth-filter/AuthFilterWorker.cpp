#include "AuthFilterWorker.h"
#include "TrapInfoList.h"
#include "table/CustomerWithTrace.h"
#include <CnapsDB.h>
#include <extra/ScopeExit.h>

bool
AuthFilterWorker::set_customer_info(filter_info_t &filter, const SysDateTime &recv_time)
{
  std::string spam_pattern = "not found destinationMdn: " + filter.messageInfo.destinationMdn;
  try
  {
    MariaStatement stmt(cnaps_db, CustomerWithTrace::query);
    stmt << filter.messageInfo.destinationMdn;

    MariaResultSet rs = stmt.execute_query();
    while (rs.next() == true)
    {
      rs >> filter.customerInfo;
      return true;
    }
  }
  catch (sql::SQLException &e)
  {
    spam_pattern = e.what();
  }

  filter.resultInfo.spamPattern1 = spam_pattern;
  to_result_nats(filter, recv_time, SMPP_RESULT_HAM, TRANS_RESULT_CODE_HAM_FAIL, SYSTEM_DB_ERROR);
  return false;
}

void
AuthFilterWorker::handle_filter(filter_info_t &filter, const std::string &subject, const SysDateTime &recv_time)
{
  ap_log.debug() << subject;

  if (handle_discard(filter, recv_time) == true)
    return;

  // ASIS: QUERY_TRAP_INFO
  if (trap_info_list.contains(filter.messageInfo.destinationMdn) == true)
  {
    filter.resultInfo.spamPattern1 = filter.messageInfo.destinationMdn;
    to_result_nats(filter, recv_time, SMPP_RESULT_SPAM, TRANS_RESULT_CODE_SPAM, F01_TRAP_CUST_SPAM);
    return;
  }

  // ASIS: QUERY_CUST_INFO
  if (set_customer_info(filter, recv_time) == false)
    return;

  // trace 이면 보내기 : ASIS 사용자 정보 조회 후 trace 번호가 아닌 경우는 과부하 체크를 한다.
  if (filter.customerInfo.traceFlag != 0)
  {
    to_next_nats(filter, recv_time);
    return;
  }

  to_next_nats(filter, recv_time);
}

