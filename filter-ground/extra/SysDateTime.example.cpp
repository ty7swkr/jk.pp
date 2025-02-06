#include <extra/SysDateTimeDiff.h>
#include <iostream>
// SysTime 예제
void sys_time_example()
{
  // 기본 생성
  SysTime time_basic(13, 30, 45);  // 13:30:45
  std::cout << "Basic time: " << time_basic.to_string() << "\n";  // 13:30:45

  // 현재 시간
  SysTime time_now = SysTime::now();
  std::cout << "Current time: " << time_now.to_string() << "\n";  // 현재 시간 출력

  // 시간 연산 (24시간 wrap)
  SysTime time_calc(23, 30, 00);
  time_calc += SysTime::Hour(2);  // 23:30 + 2시간 = 01:30
  std::cout << "Time after +2 hours: " << time_calc.to_string() << "\n";  // 01:30:00

  // 밀리초 포함 출력
  std::cout << "Time with ms: " << time_calc.to_string("%H:%M:%S.%L") << "\n";  // 01:30:00.000
}

// SysDate 예제
void sys_date_example()
{
  // 기본 날짜
  SysDate date_basic(2024, 11, 25);
  std::cout << "Basic date: " << date_basic.to_string() << "\n";  // 2024-11-25

  // 오늘/어제
  SysDate date_today = SysDate::today();
  SysDate date_yesterday = SysDate::yesterday();
  std::cout << "Today: " << date_today.to_string() << "\n"
            << "Yesterday: " << date_yesterday.to_string() << "\n";

  // 날짜 연산
  SysDate date_calc = date_basic;
  date_calc += SysDate::Day(5);
  std::cout << "Date after +5 days: " << date_calc.to_string() << "\n";  // 2024-11-30

  // 월의 첫날/마지막날
  std::cout << "First day of month: " << date_calc.first_day_of_month().to_string() << "\n"  // 2024-11-01
            << "Last day of month: " << date_calc.last_day_of_month().to_string() << "\n";    // 2024-11-30
}

// SysDateTime 예제
void sys_date_time_example()
{
  // 현재시간 가져오기.
  auto now = SysDateTime::now(); // 나노초 시간.

  // 시간 변환
  time_t          t_now   = now.to_time_t();
  std::tm         tm_now  = now.to_tm();
  struct timespec ts      = now.to_timespec();
  struct timeval  tv      = now.to_timeval();

  // 다른 형식의 시간값을 이용한 생성.
  SysDateTime dt1(t_now);                             // time_t로 생성
  SysDateTime dt2(tm_now);                            // std::tm으로 생성
  SysDateTime dt3(ts);                                // timespec으로 생성
  SysDateTime dt4(tv);                                // timeval로 생성
  SysDateTime dt5(SysDate(2024, 11, 22));             // SysDate로 생성
  SysDateTime dt6(SysDate(2024, 11, 22), SysTime(15,30,45));  // SysDate + SysTime으로 생성

  // 다른 형식의 시간값 적용
  dt1 = t_now;
  dt2 = tm_now;
  dt3 = ts;
  dt4 = tv;
  dt5 = SysDate(2024, 11, 11);
  dt6 = SysTime(11, 11, 11, 3792839);
  dt1 = SysTime::Hour(23);
  dt1 = SysTime::Microsec(123456);

  // 시간 정보 얻기
  int64_t nano  = dt1.nanosec();                      // 나노초 얻기
  int64_t micro = dt1.microsec();                     // 마이크로초 얻기
  int64_t dow   = dt1.day_of_week();                  // 요일 얻기 (0=일요일)

  // 포맷 문자열 출력
  std::string fmt1 = dt1.strftime("%Y년 %m월 %d일");    // "2024년 11월 22일"
  std::string fmt2 = dt1.strftime("%H시 %M분 %S.%L초"); // "15시 30분 45.123초"
  std::string fmt3 = dt1.strftime("%y-%m-%d");         // "24-11-22"

  // 세부 시간 설정
  SysDateTime custom;
  custom.year(2024);          // 년도만 설정
  custom.month(11);           // 월만 설정
  custom.day(22);             // 일만 설정
  custom.hour(15);            // 시만 설정
  custom.min(30);             // 분만 설정
  custom.sec(45);             // 초만 설정
  custom.millisec(123);       // 밀리초만 설정
  custom.microsec(123456);    // 마이크로초만 설정
  custom.nanosec(123456789);  // 나노초만 설정

  // 체이닝 방식 시간 설정.
  custom.year(2025).hour(1).nanosec(12231);

  // 날짜/시간 범위 함수들
  SysDateTime dt;
  dt.datetime(2024, 11, 22, 15, 30, 45, SysTime::Millisec(123));

  SysDateTime first_time        = dt.first_time_of_day();     // 2024-11-22 00:00:00.000
  SysDateTime last_time         = dt.last_time_of_day();      // 2024-11-22 23:59:59.999
  SysDateTime first_day         = dt.first_day_of_month();    // 2024-11-01 15:30:45.123
  SysDateTime last_day          = dt.last_day_of_month();     // 2024-11-30 15:30:45.123
  SysDateTime first_time_month  = dt.first_time_of_month();   // 2024-11-01 00:00:00.000
  SysDateTime last_time_month   = dt.last_time_of_month();    // 2024-11-30 23:59:59.999

  // 시간 가감
  SysDateTime calc = dt;
  calc += SysDate::Year(1);           // 1년 후
  calc -= SysDate::Month(2);          // 2달 전
  calc += SysDate::Day(5);            // 5일 후
  calc -= SysTime::Hour(3);           // 3시간 전
  calc += SysTime::Min(30);           // 30분 후
  calc -= SysTime::Sec(15);           // 15초 전
  calc += SysTime::Millisec(500);     // 500밀리초 후
  calc -= SysTime::Microsec(1000);    // 1000마이크로초 전
  calc += SysTime::Nanosec(1000000);  // 1000000나노초 후

  // 문자열 파싱
  SysDateTime parsed1 = SysDateTime::from_string("2024-11-22 15:30:45");
  SysDateTime parsed2 = SysDateTime::from_string("24/11/22 15:30", "%y/%m/%d %H:%M");
  SysDateTime parsed3 = SysDateTime::from_string_compact("2024-11-22 15:30:45.123", "%Y-%m-%d.%L"); // %L(msec) %K(usec) %N(nano sec)

  // 비교 연산자
  SysDateTime t1 = SysDateTime::now();
  SysDateTime t2 = t1 + SysTime::Hour(1);
  bool less           = t1 <  t2;
  bool greater        = t1 >  t2;
  bool equal          = t1 == t1;
  bool not_equal      = t1 != t2;
  bool less_equal     = t1 <= t1;
  bool greater_equal  = t2 >= t1;
}

// SysDateTimeDiff 예제
void sys_date_time_diff_example()
{
  SysDateTime small  = SysDateTime::now();
  SysDateTime big    = small + SysTime::Hour(2) + SysTime::Min(30) + SysTime::Sec(15);

  { // 두 시간차
    SysDateTimeDiff time_diff = big - small;

    // 시간차를 아래 단위로 환산해서
    std::cout << std::fixed << std::setprecision(9) << "total days    " << time_diff.days()    << "\n";
    std::cout << std::fixed << std::setprecision(9) << "total hours   " << time_diff.hours()   << "\n";
    std::cout << std::fixed << std::setprecision(9) << "total minutes " << time_diff.minutes() << "\n";
    std::cout << std::fixed << std::setprecision(9) << "total seconds " << time_diff.seconds() << "\n\n";

    // 시간차를 각 요소 별로
    std::cout << "Formatted diff: " << time_diff.components.to_string() << "\n";  // n days h:m:ssec.msec usec nanosec
    std::cout << "days            " << time_diff.components.days()    << "\n";    // x일
    std::cout << "hours           " << time_diff.components.hours()   << "\n";    // n시간
    std::cout << "minutes         " << time_diff.components.minutes() << "\n";    // m분
    std::cout << std::fixed << std::setprecision(9) << "seconds       " << time_diff.components.seconds() << "\n\n";
  }

  { // 두 시간차의 절대값.
    SysDateTimeDiff time_diff_abs = abs(small - big);

    // 시간차를 아래 단위로 환산해서
    std::cout << std::fixed << std::setprecision(9) << "total days    " << time_diff_abs.days()    << "\n";   // d
    std::cout << std::fixed << std::setprecision(9) << "total hours   " << time_diff_abs.hours()   << "\n";   // h
    std::cout << std::fixed << std::setprecision(9) << "total minutes " << time_diff_abs.minutes() << "\n";   // m
    std::cout << std::fixed << std::setprecision(9) << "total seconds " << time_diff_abs.seconds() << "\n\n"; // s

    // 시간차를 각 요소 별로  보여줌.
    std::cout << "Formatted diff: " << time_diff_abs.components.to_string() << "\n";  // n days h:m:ssec.msec usec nanosec
    std::cout << "days            " << time_diff_abs.components.days()    << "\n";    // n 일
    std::cout << "hours           " << time_diff_abs.components.hours()   << "\n";    // h 시간
    std::cout << "minutes         " << time_diff_abs.components.minutes() << "\n";    // m 분
    std::cout << std::fixed << std::setprecision(9) << "seconds       " << time_diff_abs.components.seconds() << "\n\n"; // s 초
  }
}

int main()
{
  std::cout << "=== SysTime Example ===\n";
  sys_time_example();

  std::cout << "\n=== SysDate Example ===\n";
  sys_date_example();

  std::cout << "\n=== SysDateTime Example ===\n";
  sys_date_time_example();

  std::cout << "\n=== SysDateTimeDiff Example ===\n";
  sys_date_time_diff_example();

  return 0;
}














