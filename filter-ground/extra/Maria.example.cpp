#include <CnapsDB.h>

#include <extra/MariaResultSet.h>
#include <extra/MariaStatement.h>
#include <extra/MariaCallableStatement.h>
#include <extra/ScopeExit.h>
#include <extra/SysDateTimeDiff.h>

#include <iostream>

int main()
{
  // MariaDB Connector/C++ URL 형식 사용, 여기 jdbc가 맞음.
  sql::SQLString  url("jdbc:mariadb://1.1.1.8:3306/mytest");
  sql::Properties properties(
  {
    {"user",      "mytest"},
    {"password",  "123qwe"}
  });

  cnaps_db.set_connection_info(url, properties);
  cnaps_db.start();
  SCOPE_EXIT({cnaps_db.stop();});

  try
  {
//    mariadb 프로시져 다루기.
//    가정한 프로시져.
//
//    CREATE PROCEDURE calculate_user_stats(
//        IN  user_id INT,                    -- 입력: 사용자 ID
//        IN  calc_date DATE,                 -- 입력: 계산 기준일
//        OUT total_orders INT,               -- 출력: 총 주문 수
//        OUT total_amount DECIMAL(10,2)      -- 출력: 총 주문 금액
//    )
//    BEGIN
//
//        -- 계산된 결과도 조회
//        SELECT
//            u.column1 ,
//            u.column2 ,
//            column1 as order_count,
//            column1 as amount
//        FROM tb_test u
//        WHERE u.column1 = user_id;
//    END
    {
      // 프로시저 호출(<mariadb/conncpp.hpp> : unstable)
      MariaCallableStatement sp(cnaps_db, "{CALL calculate_user_stats(?, ?)}");

      // IN 파라미터 설정
      sp.in_param(2);
//      sp.in_param("2024-03-20");

      // OUT 파라미터 등록
      auto out_orders = sp.out_param<int32_t>();
//      sfs_log.info();
//      auto out_amount = sp.out_param<double>();

      MariaResultSet rs = sp.execute_query();

      // OUT 파라미터 값 받기
      sfs_log.info() << out_orders.index;
      int32_t total_orders = sp.get_out_param(out_orders);
//      sfs_log.info();
//      double  total_amount = sp.get_out_param(out_amount);

//      sfs_log.info() << total_orders; // << total_amount;

      // SELECT 결과 처리
      while (rs.next())
      {
        int32_t result_3 = rs[1];
//        double  result_4 = rs["total_amount"];
        // 결과 처리...
      }
    }

    int32_t column1_value = 0;
    // SELECT 예제
    {
      sfs_log.info() << "select";
      MariaStatement selector(cnaps_db, "SELECT column1, column2, column3 FROM tb_test");
      MariaResultSet result = selector.execute_query(1000); // 1000 = fetch_size, 기본값은 1000

      while (result.next() == true)
      {
        int         column1 = result["column1"];
        std::string column2 = result[2];
        SysDateTime column3 = result[3];

        sfs_log.info() << column1;
        sfs_log.info() << column2;
        sfs_log.info() << result[3].is_null() << column3.to_string();

        column1_value = column1;
      }
    }

    // CHECKSUM 예제
    {
      sfs_log.info() << "checksum";
      MariaStatement  checksum(cnaps_db, "CHECKSUM TABLE tb_test");
      MariaResultSet  result = checksum.execute_query();

      while (result.next() == true)
        sfs_log.info() << "table checksum" << result["Checksum"].as_int64();
    }

    // INSERT 예제
    {
      sfs_log.info() << "insert";
      MariaStatement inserter(cnaps_db, "INSERT INTO tb_test (column1, column2) VALUES (?, ?)");
      inserter << column1_value+1 << "홍길동";
      inserter.execute_update();
      sfs_log.info() << "Insert 완료";
    }

    // CHECKSUM
    {
      sfs_log.info() << "select";
      MariaStatement  selector(cnaps_db, "CHECKSUM TABLE tb_test");
      MariaResultSet  result = selector.execute_query();

      while (result.next() == true)
        sfs_log.info() << "table checksum" << result["Checksum"].as_int64();
    }

    // UPDATE 예제
    {
      sfs_log.info() << "update";
      MariaStatement updater(cnaps_db, "UPDATE tb_test SET column2 = ? WHERE column1 = ?");

      updater << "김철수" << 1;
      int rows_affected = updater.execute_update();
      sfs_log.info() << rows_affected << "개 행이 수정됨";
    }
    // CHECKSUM
    {
      sfs_log.info() << "checksum";
      MariaStatement  checksum(cnaps_db, "CHECKSUM TABLE tb_test");
      MariaResultSet  result = checksum.execute_query();

      while (result.next() == true)
        sfs_log.info() << "table checksum" << result["Checksum"].as_int64();
    }

    // DELETE 예제
    {
      sfs_log.info() << "delete";
      MariaStatement deleter(cnaps_db, "DELETE FROM tb_test WHERE column2 = ?");
      deleter << "김철수";
      int rows_affected = deleter.execute_update();
      sfs_log.info() << rows_affected << "개 행이 삭제됨";
    }

    // CHECKSUM
    {
      sfs_log.info() << "checksum";
      MariaStatement  checksum(cnaps_db, "CHECKSUM TABLE tb_test");
      MariaResultSet  result = checksum.execute_query();

      while (result.next() == true)
        sfs_log.info() << "table checksum" << result["Checksum"].as_int64();
    }
  }
  catch (sql::SQLException& e)
  {
    sfs_log.info() << "Reason   : "  << e.what();
    sfs_log.info() << "ErrorCode: "  << e.getErrorCode();
    sfs_log.info() << "SQLState : "  << e.getSQLState();
  }

  return 0;
}


