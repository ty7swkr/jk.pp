/*
 * SysDate.h
 *
 *  Created on: 2021. 7. 10.
 *      Author: tys
 */

#pragma once

#include <extra/SysTime.h>
#if __cplusplus >= 201703L
#include <optional>
#endif

/**
 * @class SysDate
 * @brief 날짜만을 다루는 클래스
 *
 * @details 주요 기능:
 * - 년/월/일 단위의 날짜 표현
 * - 날짜 연산 (년/월/일 단위)
 * - 날짜 비교
 * - 요일 확인
 * - 현재 날짜 조회
 * - 월 단위 첫날/마지막날 계산
 * - 다양한 포맷의 날짜 문자열 변환
 *
 * @note 시간을 포함한 계산이 필요한 경우 SysDateTime을 사용하세요.
 * @see SysDateTime
 * @see SysDate
 */

class SysDate
{
public:
  SysDate() {}
  SysDate(const int64_t &year, const int64_t &month, const int64_t &day) { set(year, month, day); }

  /********* classes for operation *********/
  enum class UNIT: int { YEAR = 10, MONTH, DAY };
  template<UNIT> struct Unit
  {
    Unit() : value(0) {}
    Unit(const int64_t &value) : value(value) {}
    const int64_t &operator()() const { return value; }
    int64_t value;
  };

  using Year  = Unit<UNIT::YEAR>;
  using Month = Unit<UNIT::MONTH>;
  using Day   = Unit<UNIT::DAY>;

  static SysDate now      (const int64_t &adjust_sec = 0);
  static SysDate today    () { return now(); }
  static SysDate yesterday() { return now()-SysDate::Day(1); }

  const int64_t &year () const { return year_;  }
  const int64_t &month() const { return month_; }
  const int64_t &day  () const { return day_;   }
  const int64_t &day_of_week() const { return day_of_week_; } // Day of week. [0-6], 0=SUNDAY

  SysDate &set    (const int64_t &year, const int64_t &month, const int64_t &day);
  SysDate &set_utc()                     { *this = SysDate::now(-SysTime::utc_offset_sec()); return *this; }
  SysDate &year   (const int64_t &value) { return set(value, month_, day_); }
  SysDate &month  (const int64_t &value) { return set(year_, value,  day_); }
  SysDate &day    (const int64_t &value) { return set(year_, month_, value); }

  template<typename T> SysDate &set(const T &value){ return operator= (value); }
  template<typename T> SysDate &add(const T &value){ return operator+=(value); }

  SysDate &operator=  (const Year    &rhs) { return year  (rhs());}
  SysDate &operator=  (const Month   &rhs) { return month (rhs());}
  SysDate &operator=  (const Day     &rhs) { return day   (rhs());}

  SysDate &operator+= (const Year    &rhs) { year  (year_  + rhs()); return *this; }
  SysDate &operator+= (const Month   &rhs) { month (month_ + rhs()); return *this; }
  SysDate &operator+= (const Day     &rhs) { day   (day_   + rhs()); return *this; }

  SysDate &operator-= (const Year    &rhs) { year  (year_  - rhs()); return *this; }
  SysDate &operator-= (const Month   &rhs) { month (month_ - rhs()); return *this; }
  SysDate &operator-= (const Day     &rhs) { day   (day_   - rhs()); return *this; }

  SysDate operator+   (const Year    &rhs) const { SysDate sd = *this; return sd.year (sd.year_  + rhs()); }
  SysDate operator+   (const Month   &rhs) const { SysDate sd = *this; return sd.month(sd.month_ + rhs()); }
  SysDate operator+   (const Day     &rhs) const { SysDate sd = *this; return sd.day  (sd.day_   + rhs()); }
  SysDate operator-   (const Year    &rhs) const { SysDate sd = *this; return sd.year (sd.year_  - rhs()); }
  SysDate operator-   (const Month   &rhs) const { SysDate sd = *this; return sd.month(sd.month_ - rhs()); }
  SysDate operator-   (const Day     &rhs) const { SysDate sd = *this; return sd.day  (sd.day_   - rhs()); }

  bool    operator==  (const SysDate &rhs) const;
  bool    operator!=  (const SysDate &rhs) const;
  bool    operator<=  (const SysDate &rhs) const;
  bool    operator>=  (const SysDate &rhs) const;
  bool    operator<   (const SysDate &rhs) const;
  bool    operator>   (const SysDate &rhs) const;
  bool    is_null     () const { return null_ == true; }
  bool    is_not_null () const { return null_ != true; }

  SysDate first_day_of_month() const;
  SysDate last_day_of_month () const;

  std::string to_string(const std::string &format = "%Y-%m-%d") const { return strftime(format); }
  std::string strftime (const std::string &format = "%Y-%m-%d") const;
  std::string strstime (const std::string &format = "%y-%m-%d") const { return strftime(format); }
  bool        strptime (const std::string &time_string,
                        const std::string &format = "%Y-%m-%d");

  static SysDate from_string(const std::string &time_string, const std::string &format = "%Y-%m-%d");
#if __cplusplus >= 201703L
  static std::optional<SysDate>
                 from_string_f(const std::string &time_string, const std::string &format = "%Y-%m-%d");
#endif

private:
  static std::string
  to_stringf(const int64_t &value, const std::string &format = "%03ll")
  {
    int size_f = std::snprintf(nullptr, 0, format.c_str(), value) + 1;
    if (size_f <= 0) return "";

    char buff[size_f] = { 0, };
    std::snprintf(buff, sizeof(buff), format.c_str(), value);
    return buff;
  }

  int64_t year_  = 1970;
  int64_t month_ = 1;
  int64_t day_   = 1;
  int64_t day_of_week_ = 0; // Day of week. [0-6], 0=SUNDAY
  bool    null_  = true;
};

inline std::string
SysDate::strftime(const std::string &format) const
{
  std::tm tm;
  memset(&tm, 0x00, sizeof(std::tm));

  tm.tm_year = year_  - 1900;
  tm.tm_mon  = month_ - 1;
  tm.tm_mday = day_;

  std::ostringstream str_time;
  str_time << std::put_time(&tm, format.c_str());
  return str_time.str();
}

inline bool
SysDate::strptime(const std::string &time_string,
                  const std::string &format)
{
  std::tm tm;
  memset(&tm, 0x00, sizeof(tm));

  std::istringstream ss(time_string);

  ss >> std::get_time(&tm, format.c_str());

  if (ss.fail() == true)
    return false;

  null_ = false;
  year_ = tm.tm_year + 1900;
  month_= tm.tm_mon + 1;
  day_  = tm.tm_mday;
  day_of_week_ = tm.tm_wday;
  return true;
}

inline SysDate
SysDate::from_string(const std::string &time_string,
                     const std::string &format)
{
  std::tm tm;
  memset(&tm, 0x00, sizeof(tm));

  std::istringstream ss(time_string);

  ss >> std::get_time(&tm, format.c_str());

  SysDate date;
  if (ss.fail() == true)
    return date;

  date.null_  = false;
  date.year_  = tm.tm_year + 1900;
  date.month_ = tm.tm_mon + 1;
  date.day_   = tm.tm_mday;
  date.day_of_week_ = tm.tm_wday;
  return date;
}

#if __cplusplus >= 201703L
inline std::optional<SysDate>
SysDate::from_string_f(const std::string &time_string,
                       const std::string &format)
{
  std::tm tm;
  memset(&tm, 0x00, sizeof(tm));

  std::istringstream ss(time_string);

  ss >> std::get_time(&tm, format.c_str());

  if (ss.fail() == true)
    return std::nullopt;

  SysDate date;
  date.null_  = false;
  date.year_  = tm.tm_year + 1900;
  date.month_ = tm.tm_mon + 1;
  date.day_   = tm.tm_mday;
  date.day_of_week_ = tm.tm_wday;
  return date;
}
#endif

inline SysDate
SysDate::first_day_of_month() const
{
  SysDate sd = *this;
  sd.set(year_, month_, 1);
  return sd;
}

inline SysDate
SysDate::last_day_of_month() const
{
#define SysDate_LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))

  static const int64_t ytab[2][12] =
  {
   { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
   { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };

  SysDate st = *this;

  st.day_ = ytab[SysDate_LEAPYEAR(year_-1900)][month_-1];
  st.set(st.year_, st.month_, st.day_);

  return st;
}

inline SysDate
SysDate::now(const int64_t &adjust_sec)
{
  time_t from_time_t = std::time(nullptr) + adjust_sec;

  std::tm to;
  localtime_r(&from_time_t, &to);

  return SysDate(to.tm_year + 1900,
                 to.tm_mon  + 1,
                 to.tm_mday);
}

inline SysDate &
SysDate::set(const int64_t &year, const int64_t &month, const int64_t &day)
{
  std::tm from;
  memset(&from, 0x00, sizeof(std::tm));

  from.tm_year = year  - 1900;
  from.tm_mon  = month - 1;
  from.tm_mday = day;

  time_t from_time_t = std::mktime(&from);

  std::tm to;
  localtime_r(&from_time_t, &to);

  year_  = to.tm_year + 1900;
  month_ = to.tm_mon  + 1;
  day_   = to.tm_mday;
  day_of_week_ = to.tm_wday;
  null_  = false;

  return *this;
}

inline bool
SysDate::operator==(const SysDate &rhs) const
{
  return year_ == rhs.year_ && month_ == rhs.month_ && day_ == rhs.day_;
}

inline bool
SysDate::operator!=(const SysDate &rhs) const
{
  return !(*this == rhs);
}

inline bool
SysDate::operator<=(const SysDate &rhs) const
{
  if (year_ == rhs.year_ && month_ == rhs.month_) return day_ <= rhs.day_;
  if (year_ == rhs.year_) return month_ <= rhs.month_;
  return year_ <= rhs.year_;
}

inline bool
SysDate::operator>=(const SysDate &rhs) const
{
  if (year_ == rhs.year_ && month_ == rhs.month_) return day_ >= rhs.day_;
  if (year_ == rhs.year_) return month_ >= rhs.month_;
  return year_ >= rhs.year_;
}

inline bool
SysDate::operator< (const SysDate &rhs) const
{
  if (year_ == rhs.year_ && month_ == rhs.month_) return day_ < rhs.day_;
  if (year_ == rhs.year_) return month_ < rhs.month_;
  return year_ < rhs.year_;
}

inline bool
SysDate::operator> (const SysDate &rhs) const
{
  if (year_ == rhs.year_ && month_ == rhs.month_) return day_ > rhs.day_;
  if (year_ == rhs.year_) return month_ > rhs.month_;
  return year_ > rhs.year_;
}



