#pragma once

#include <extra/SysDate.h>
#include <extra/SysTime.h>

/**
 * @class SysDateTime
 * @brief 날짜와 시간을 모두 다루는 통합 클래스
 *
 * SysDate와 SysTime의 기능을 모두 포함,
 *
 * @details 주요 기능:
 * - 날짜와 시간의 통합 처리
 * - 년/월/일/시/분/초/나노초 단위의 연산
 * - 현재 날짜/시간 조회
 * - UTC/로컬 시간 변환
 * - 날짜/시간 분리 및 변환
 * - 다양한 포맷의 문자열 변환
 * - 시간대 처리
 * - 밀리초/마이크로초/나노초 단위 시간 처리
 */
class SysDateTime
{
public:
  static SysDateTime now(const int64_t &adjust_sec = 0) { return SysDateTime().current(adjust_sec); }

  /********* constructor *********/
  SysDateTime() {}
  SysDateTime(const time_t           &value) { *this = value; } // nanosec is zero
  SysDateTime(const std::tm          &value) { *this = value; } // nanosec is zero
  SysDateTime(const struct timespec  &value) { *this = value; }
  SysDateTime(const struct timeval   &value) { *this = value; }
  SysDateTime(const std::chrono::system_clock::time_point &value) : clock_(value), null_(false) {}
  SysDateTime(const SysDate          &value) { date(value).time(0, 0, 0); } // nanosec is zero
  SysDateTime(const SysDate          &date,
              const SysTime          &time)  { this->date(date); this->time(time); } // nanosec is zero
  SysDateTime(const SysDateTime      &value) { *this = value; }

  /********* returns the time value. *********/
  int64_t year()     const;
  int64_t month()    const;
  int64_t day()      const;
  int64_t hour()     const;
  int64_t min()      const;
  int64_t sec()      const;
  int64_t millisec() const;
  int64_t microsec() const;
  int64_t nanosec()  const;
  const std::chrono::system_clock::time_point &
          time_point() const { return clock_; }

  SysTime time() const;
  SysDate date() const;

  // 1970.1.1 00:00:00 이후 각 단위로 환산된값.
  struct duration_t
  {
    duration_t(const std::chrono::system_clock::time_point &clock) : clock(clock) {}
    double  days      () const { return hours   () / 24.; }
    double  hours     () const { return mins    () / 60.; }
    double  mins      () const { return seconds () / 60.; }
    double  seconds   () const { return nanosecs() / 1000000000.; }
    double  millisecs () const { return nanosecs() / 1000000.; }
    double  microsecs () const { return nanosecs() / 1000.;}
    int64_t nanosecs  () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(clock.time_since_epoch()).count(); }

    std::chrono::system_clock::time_point clock;
  };

  duration_t duration() const { return duration_t(this->clock_); }

  // SysDate::Day of week. [0-6]
  // 0sun 1mon 2tue 3wen 4thu 5fri 6sat
  int64_t day_of_week() const;

  /********* conversion *********/
  time_t          to_time_t   () const;
  std::tm         to_tm       () const;
  struct timespec to_timespec () const;
  struct timeval  to_timeval  () const;
  // strftime format + %L(msec) %K(usec) %N(nano sec)
  std::string     strftime    (std::string format = "%Y-%m-%d %H:%M:%S") const;
  std::string     strstime    (std::string format = "%y-%m-%d %H:%M:%S") const    { return strftime(format); }
  std::string     to_string   (std::string format = "%Y-%m-%d %H:%M:%S.%L") const { return strftime(format); }
  SysDateTime     to_utc      () { SysDateTime utc   = *this; utc   -= SysTime::Sec(SysTime::utc_offset_sec()); return utc; }
  SysDateTime     to_local    () { SysDateTime local = *this; local += SysTime::Sec(SysTime::utc_offset_sec()); return local; }

  /********* parsing *********/
  bool strptime   (const std::string &str,
                   const std::string &format = "%Y-%m-%d %H:%M:%S");

  bool is_null    () const { return  null_; }
  bool is_not_null() const { return !null_; }

  static SysDateTime
  from_string               (const std::string &str,
                             const std::string &format = "%Y-%m-%d %H:%M:%S");
  // unstable, only %y,%Y,%m,%d,%H,%M,%S,%L(msec) %K(usec) %N(nano sec)
  static SysDateTime
  from_string_compact       (const std::string &str,
                             const std::string &format = "%Y-%m-%d %H:%M:%S");

  /********* set time *********/
  SysDateTime &null     () { null_ = true; clock_ = std::chrono::system_clock::time_point{}; return *this; }
  SysDateTime &current  (const int64_t &adjust_sec = 0);
  SysDateTime &utc      () { *this -= SysTime::Sec(SysTime::utc_offset_sec()); return *this; }
  SysDateTime &date     (const int64_t &year, const int64_t &month, const int64_t &day);
  SysDateTime &date     (const SysDate &date) { return this->date(date.year(), date.month(), date.day()); }

  SysDateTime &time     (const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Millisec &millisec);
  SysDateTime &time     (const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Microsec &microsec);
  SysDateTime &time     (const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Nanosec  &nanosec = SysTime::Nanosec(0));
  SysDateTime &time     (const SysTime &time) { return this->time(time.hour(), time.min(), time.sec(), SysTime::Nanosec(time.nanosec())); }

  SysDateTime &datetime (const int64_t &year, const int64_t &month, const int64_t &day,
                         const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Millisec &millisec);
  SysDateTime &datetime (const int64_t &year, const int64_t &month, const int64_t &day,
                         const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Microsec &microsec);
  SysDateTime &datetime (const int64_t &year, const int64_t &month, const int64_t &day,
                         const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Nanosec  &nanosec = SysTime::Nanosec(0));

  SysDateTime &year     (const int64_t &year);
  SysDateTime &month    (const int64_t &month);
  SysDateTime &day      (const int64_t &day);
  SysDateTime &hour     (const int64_t &hour);
  SysDateTime &min      (const int64_t &min);
  SysDateTime &sec      (const int64_t &sec);
  SysDateTime &millisec (const int64_t &millisec);
  SysDateTime &microsec (const int64_t &microsec);
  SysDateTime &nanosec  (const int64_t &nanosec);

  template<typename T> SysDateTime &set(const T &value){ return operator= (value); }
  template<typename T> SysDateTime &add(const T &value){ return operator+=(value); }

  SysDateTime first_time_of_day   () const;
  SysDateTime first_day_of_month  () const;
  SysDateTime first_time_of_month () const;

  SysDateTime last_time_of_day    () const;
  SysDateTime last_day_of_month   () const;
  SysDateTime last_time_of_month  () const;

  /********* operator *********/
  SysDateTime &operator= (const SysDateTime       &rhs) { clock_ = rhs.clock_; null_ = rhs.null_; return *this; }
  SysDateTime &operator= (const SysDate           &rhs) { return date     (rhs);}
  SysDateTime &operator= (const SysTime           &rhs) { return time     (rhs);}
  SysDateTime &operator= (const SysDate::Year     &rhs) { return year     (rhs());}
  SysDateTime &operator= (const SysDate::Month    &rhs) { return month    (rhs());}
  SysDateTime &operator= (const SysDate::Day      &rhs) { return day      (rhs());}
  SysDateTime &operator= (const SysTime::Hour     &rhs) { return hour     (rhs());}
  SysDateTime &operator= (const SysTime::Min      &rhs) { return min      (rhs());}
  SysDateTime &operator= (const SysTime::Sec      &rhs) { return sec      (rhs());}
  SysDateTime &operator= (const SysTime::Millisec &rhs) { return millisec (rhs());}
  SysDateTime &operator= (const SysTime::Microsec &rhs) { return microsec (rhs());}
  SysDateTime &operator= (const SysTime::Nanosec  &rhs) { return nanosec  (rhs());}
  SysDateTime &operator= (const time_t            &rhs) { clock_ = time_t_to_time_point(rhs); null_ = false; return *this; }
  SysDateTime &operator= (std::tm                  rhs) { clock_ = time_t_to_time_point(std::mktime(&rhs)); null_ = false; return *this; }
  SysDateTime &operator= (const struct timespec   &rhs) { clock_ = nano_time_t_to_time_point (rhs.tv_sec, rhs.tv_nsec); null_ = false; return *this; }
  SysDateTime &operator= (const struct timeval    &rhs) { clock_ = micro_time_t_to_time_point(rhs.tv_sec, rhs.tv_usec); null_ = false; return *this; }

  SysDateTime &operator+=(const SysDate::Year     &rhs) { year  (year () + rhs()); return *this;  }
  SysDateTime &operator+=(const SysDate::Month    &rhs) { month (month() + rhs()); return *this;  }
  SysDateTime &operator+=(const SysDate::Day      &rhs) { clock_ += std::chrono::hours        (rhs()*24); null_ = false; return *this; }
  SysDateTime &operator+=(const SysTime::Hour     &rhs) { clock_ += std::chrono::hours        (rhs());    null_ = false; return *this; }
  SysDateTime &operator+=(const SysTime::Min      &rhs) { clock_ += std::chrono::minutes      (rhs());    null_ = false; return *this; }
  SysDateTime &operator+=(const SysTime::Sec      &rhs) { clock_ += std::chrono::seconds      (rhs());    null_ = false; return *this; }
  SysDateTime &operator+=(const SysTime::Millisec &rhs) { clock_ += std::chrono::milliseconds (rhs());    null_ = false; return *this; }
  SysDateTime &operator+=(const SysTime::Microsec &rhs) { clock_ += std::chrono::microseconds (rhs());    null_ = false; return *this; }
  SysDateTime &operator+=(const SysTime::Nanosec  &rhs) { clock_ += std::chrono::nanoseconds  (rhs());    null_ = false; return *this; }

  SysDateTime &operator-=(const SysDate::Year     &rhs) { year  (year () - rhs()); return *this; }
  SysDateTime &operator-=(const SysDate::Month    &rhs) { month (month() - rhs()); return *this; }
  SysDateTime &operator-=(const SysDate::Day      &rhs) { clock_ -= std::chrono::hours        (rhs()*24); null_ = false; return *this; }
  SysDateTime &operator-=(const SysTime::Hour     &rhs) { clock_ -= std::chrono::hours        (rhs());    null_ = false; return *this; }
  SysDateTime &operator-=(const SysTime::Min      &rhs) { clock_ -= std::chrono::minutes      (rhs());    null_ = false; return *this; }
  SysDateTime &operator-=(const SysTime::Sec      &rhs) { clock_ -= std::chrono::seconds      (rhs());    null_ = false; return *this; }
  SysDateTime &operator-=(const SysTime::Millisec &rhs) { clock_ -= std::chrono::milliseconds (rhs());    null_ = false; return *this; }
  SysDateTime &operator-=(const SysTime::Microsec &rhs) { clock_ -= std::chrono::microseconds (rhs());    null_ = false; return *this; }
  SysDateTime &operator-=(const SysTime::Nanosec  &rhs) { clock_ -= std::chrono::nanoseconds  (rhs());    null_ = false; return *this; }

  SysDateTime  operator+ (const SysDate::Year     &rhs) const { SysDateTime st = *this; return st.year  (st.year()  + rhs()); }
  SysDateTime  operator+ (const SysDate::Month    &rhs) const { SysDateTime st = *this; return st.month (st.month() + rhs()); }
  SysDateTime  operator+ (const SysDate::Day      &rhs) const { SysDateTime st = *this; st.clock_ += std::chrono::hours       (rhs()*24); st.null_ = false; return st; }
  SysDateTime  operator+ (const SysTime::Hour     &rhs) const { SysDateTime st = *this; st.clock_ += std::chrono::hours       (rhs());    st.null_ = false; return st; }
  SysDateTime  operator+ (const SysTime::Min      &rhs) const { SysDateTime st = *this; st.clock_ += std::chrono::minutes     (rhs());    st.null_ = false; return st; }
  SysDateTime  operator+ (const SysTime::Sec      &rhs) const { SysDateTime st = *this; st.clock_ += std::chrono::seconds     (rhs());    st.null_ = false; return st; }
  SysDateTime  operator+ (const SysTime::Millisec &rhs) const { SysDateTime st = *this; st.clock_ += std::chrono::milliseconds(rhs());    st.null_ = false; return st; }
  SysDateTime  operator+ (const SysTime::Microsec &rhs) const { SysDateTime st = *this; st.clock_ += std::chrono::microseconds(rhs());    st.null_ = false; return st; }
  SysDateTime  operator+ (const SysTime::Nanosec  &rhs) const { SysDateTime st = *this; st.clock_ += std::chrono::nanoseconds (rhs());    st.null_ = false; return st; }

  SysDateTime  operator- (const SysDate::Year     &rhs) const { SysDateTime st = *this; return st.year  (st.year()  - rhs()); }
  SysDateTime  operator- (const SysDate::Month    &rhs) const { SysDateTime st = *this; return st.month (st.month() - rhs()); }
  SysDateTime  operator- (const SysDate::Day      &rhs) const { SysDateTime st = *this; st.clock_ -= std::chrono::hours       (rhs()*24); st.null_ = false; return st; }
  SysDateTime  operator- (const SysTime::Hour     &rhs) const { SysDateTime st = *this; st.clock_ -= std::chrono::hours       (rhs());    st.null_ = false; return st; }
  SysDateTime  operator- (const SysTime::Min      &rhs) const { SysDateTime st = *this; st.clock_ -= std::chrono::minutes     (rhs());    st.null_ = false; return st; }
  SysDateTime  operator- (const SysTime::Sec      &rhs) const { SysDateTime st = *this; st.clock_ -= std::chrono::seconds     (rhs());    st.null_ = false; return st; }
  SysDateTime  operator- (const SysTime::Millisec &rhs) const { SysDateTime st = *this; st.clock_ -= std::chrono::milliseconds(rhs());    st.null_ = false; return st; }
  SysDateTime  operator- (const SysTime::Microsec &rhs) const { SysDateTime st = *this; st.clock_ -= std::chrono::microseconds(rhs());    st.null_ = false; return st; }
  SysDateTime  operator- (const SysTime::Nanosec  &rhs) const { SysDateTime st = *this; st.clock_ -= std::chrono::nanoseconds (rhs());    st.null_ = false; return st; }

  bool operator< (const SysDateTime &rhs) const { return clock_ <  rhs.clock_; }
  bool operator> (const SysDateTime &rhs) const { return clock_ >  rhs.clock_; }
  bool operator<=(const SysDateTime &rhs) const { return clock_ <= rhs.clock_; }
  bool operator>=(const SysDateTime &rhs) const { return clock_ >= rhs.clock_; }
  bool operator==(const SysDateTime &rhs) const { return clock_ == rhs.clock_; }
  bool operator!=(const SysDateTime &rhs) const { return clock_ != rhs.clock_; }

protected:
  enum
  {
    MILLISEC  = 1000LL,
    MICROSEC  = 1000000LL,
    NANOSEC   = 1000000000LL
  };

  static std::chrono::system_clock::time_point
  time_t_to_time_point(const int64_t &time_t)
  {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::seconds(time_t));
  }

  static std::chrono::system_clock::time_point
  milli_time_t_to_time_point(const int64_t &time_t, const int64_t &millisec)
  {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::milliseconds(time_t * MILLISEC + millisec));
  }

  static std::chrono::system_clock::time_point
  micro_time_t_to_time_point(const int64_t &time_t, const int64_t &microsec)
  {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::microseconds(time_t * MICROSEC + microsec));
  }

  static std::chrono::system_clock::time_point
  nano_time_t_to_time_point(const int64_t &time_t, const int64_t &nanosec)
  {
    return std::chrono::time_point<std::chrono::system_clock>(std::chrono::nanoseconds(time_t * NANOSEC + nanosec));
  }

  static std::string
  to_stringf(const int64_t &value, const std::string &format = "%03ll")
  {
    int size_f = std::snprintf(nullptr, 0, format.c_str(), value) + 1;
    if (size_f <= 0) return "";

    char buff[size_f] = { 0, };
    std::snprintf(buff, sizeof(buff), format.c_str(), value);
    return buff;
  }

private:
  std::chrono::system_clock::time_point clock_;
  bool null_ = true;
};

#include <extra/SysDateTime.inc>

