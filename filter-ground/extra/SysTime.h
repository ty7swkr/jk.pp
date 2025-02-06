/*
 * SysTime.h
 *
 *  Created on: 2021. 7. 10.
 *      Author: tys
 */

#pragma once

#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <string.h>

/**
 * @class SysTime
 * @brief 하루(24시간) 내의 시간을 표현하고 연산하는 클래스
 *
 * 24시간을 초과하는 연산은 자동으로 wrap됩니다. 23h+2h = 01h
 * 날짜 계산이 필요 없는 순수 시간 처리에 사용됩니다.
 *
 * @details 주요 기능:
 * - 시/분/초/나노초 단위의 시간 표현
 * - 24시간 기반의 시간 연산 (자동 wrap)
 * - 현재 시간 조회
 * - 시간 비교 연산
 * - UTC/로컬 시간 변환
 * - 다양한 포맷의 시간 문자열 변환
 *
 * @note 날짜를 포함한 계산이 필요한 경우 SysDateTime을 사용하세요.
 * @see SysDateTime
 * @see SysDate
 */
class SysTime
{
public:
  SysTime() {}
  SysTime(const int64_t &hour, const int64_t &min, const int64_t &sec, const int64_t &nanosec = 0) { set(hour, min, sec, nanosec); }
  SysTime(const time_t  &value) { *this = value; }
  SysTime(const std::string &time_string, const std::string &format) { this->strptime(time_string, format); }

  /********* classes for operation *********/
  enum class UNIT: int { HOUR = 1, MIN, SEC, MILLISEC, MICROSEC, NANOSEC };
  template<UNIT> struct Unit { Unit():value(0) {} Unit(const int64_t &value) : value(value) {} const int64_t &operator()() const { return value; } int64_t value; };

  using Hour      = Unit<UNIT::HOUR>;
  using Min       = Unit<UNIT::MIN>;
  using Sec       = Unit<UNIT::SEC>;
  using Millisec  = Unit<UNIT::MILLISEC>;
  using Microsec  = Unit<UNIT::MICROSEC>;
  using Nanosec   = Unit<UNIT::NANOSEC>;

  static SysTime now(const int64_t &adjust_sec = 0);

  int64_t hour    () const { return hour_;  }
  int64_t min     () const { return min_;   }
  int64_t sec     () const { return sec_;   }
  int64_t millisec() const { return nano_/1000000LL; }
  int64_t microsec() const { return nano_/1000LL; }
  int64_t nanosec () const { return nano_;  }

  SysTime &set        (const int64_t  &hour, const int64_t &min, const int64_t &sec, const int64_t &nano = 0);
  SysTime &set_utc    () { *this -= SysTime::Sec(SysTime::utc_offset_sec()); return *this; }
  SysTime &hour       (const int64_t  &value) { return set(value, min_,  sec_,  nano_); }
  SysTime &min        (const int64_t  &value) { return set(hour_, value, sec_,  nano_); }
  SysTime &sec        (const int64_t  &value) { return set(hour_, min_,  value, nano_); }
  SysTime &millisec   (const int64_t  &value) { return set(hour_, min_,  sec_ , value * 1000000LL); }
  SysTime &microsec   (const int64_t  &value) { return set(hour_, min_,  sec_,  value * 1000LL); }
  SysTime &nanosec    (const int64_t  &value) { return set(hour_, min_,  sec_,  value); }

  template<typename T> SysTime &set(const T &value){ return operator= (value); }
  template<typename T> SysTime &add(const T &value){ return operator+=(value); }

  SysTime &operator=  (const Hour     &rhs) { return hour     (rhs());}
  SysTime &operator=  (const Min      &rhs) { return min      (rhs());}
  SysTime &operator=  (const Sec      &rhs) { return sec      (rhs());}
  SysTime &operator=  (const Millisec &rhs) { return millisec (rhs());}
  SysTime &operator=  (const Microsec &rhs) { return microsec (rhs());}
  SysTime &operator=  (const Nanosec  &rhs) { return nanosec  (rhs());}
  SysTime &operator=  (const time_t   &rhs);

  SysTime &operator+= (const Hour     &rhs) { hour    (hour_  + rhs()); return *this; }
  SysTime &operator+= (const Min      &rhs) { min     (min_   + rhs()); return *this; }
  SysTime &operator+= (const Sec      &rhs) { sec     (sec_   + rhs()); return *this; }
  SysTime &operator+= (const Millisec &rhs) { nanosec (nano_  + rhs() * 1000000LL); return *this; }
  SysTime &operator+= (const Microsec &rhs) { nanosec (nano_  + rhs() * 1000LL); return *this; }
  SysTime &operator+= (const Nanosec  &rhs) { nanosec (nano_  + rhs()); return *this; }

  SysTime &operator-= (const Hour     &rhs) { hour    (hour_  - rhs()); return *this; }
  SysTime &operator-= (const Min      &rhs) { min     (min_   - rhs()); return *this; }
  SysTime &operator-= (const Sec      &rhs) { sec     (sec_   - rhs()); return *this; }
  SysTime &operator-= (const Millisec &rhs) { nanosec (nano_  - rhs() * 1000000LL); return *this; }
  SysTime &operator-= (const Microsec &rhs) { nanosec (nano_  - rhs() * 1000LL); return *this; }
  SysTime &operator-= (const Nanosec  &rhs) { nanosec (nano_  - rhs()); return *this; }

  SysTime  operator+  (const Hour     &rhs) const { SysTime sd = *this; return sd.hour    (sd.hour_ + rhs()); }
  SysTime  operator+  (const Min      &rhs) const { SysTime sd = *this; return sd.min     (sd.min_  + rhs()); }
  SysTime  operator+  (const Sec      &rhs) const { SysTime sd = *this; return sd.sec     (sd.sec_  + rhs()); }
  SysTime  operator+  (const Millisec &rhs) const { SysTime sd = *this; return sd.nanosec (sd.nano_ + rhs() * 1000000LL); }
  SysTime  operator+  (const Microsec &rhs) const { SysTime sd = *this; return sd.nanosec (sd.nano_ + rhs() * 1000LL); }
  SysTime  operator+  (const Nanosec  &rhs) const { SysTime sd = *this; return sd.nanosec (sd.nano_ + rhs()); }

  SysTime  operator-  (const Hour     &rhs) const { SysTime sd = *this; return sd.hour    (sd.hour_ - rhs()); }
  SysTime  operator-  (const Min      &rhs) const { SysTime sd = *this; return sd.min     (sd.min_  - rhs()); }
  SysTime  operator-  (const Sec      &rhs) const { SysTime sd = *this; return sd.sec     (sd.sec_  - rhs()); }
  SysTime  operator-  (const Millisec &rhs) const { SysTime sd = *this; return sd.nanosec (sd.nano_ - rhs() * 1000000LL); }
  SysTime  operator-  (const Microsec &rhs) const { SysTime sd = *this; return sd.nanosec (sd.nano_ - rhs() * 1000LL); }
  SysTime  operator-  (const Nanosec  &rhs) const { SysTime sd = *this; return sd.nanosec (sd.nano_ - rhs()); }

  bool     operator== (const SysTime  &rhs) const;
  bool     operator!= (const SysTime  &rhs) const;
  bool     operator<= (const SysTime  &rhs) const;
  bool     operator>= (const SysTime  &rhs) const;
  bool     operator<  (const SysTime  &rhs) const;
  bool     operator>  (const SysTime  &rhs) const;

  std::string strftime (std::string        format = "%H:%M:%S") const;
  std::string to_string(const std::string &format = "%H:%M:%S") const { return strftime(format); }
  bool        strptime (const std::string &time_string,
                        const std::string &format = "%H:%M:%S");

  static std::tm localtime(const std::time_t &t)
  {
    std::tm r;
    localtime_r(&t, &r);
    return r;
  }

  static std::tm gmtime(const std::time_t &t)
  {
    std::tm r;
    gmtime_r(&t, &r);
    return r;
  }

  static int64_t utc_offset_sec()
  {
    return localtime(std::time(nullptr)).tm_gmtoff;
  }

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

  int64_t hour_ = 0;
  int64_t min_  = 0;
  int64_t sec_  = 0;
  int64_t nano_ = 0;
};

inline bool
SysTime::strptime(const std::string &time_string,
                  const std::string &format)
{
  std::tm tm;
  memset(&tm, 0x00, sizeof(tm));

  std::istringstream ss(time_string);
  ss >> std::get_time(&tm, format.c_str());

  if (ss.fail() == true)
    return false;

  hour_ = tm.tm_hour;
  min_  = tm.tm_min;
  sec_  = tm.tm_sec;
  nano_ = 0;

  return true;
}

inline std::string
SysTime::strftime(std::string format) const
{
  if (format.find("%L") != std::string::npos)
    format.replace(format.find("%L"), 2, to_stringf(millisec(), "%03lld"));

  if (format.find("%K") != std::string::npos)
    format.replace(format.find("%K"), 2, to_stringf(microsec(), "%06lld"));

  if (format.find("%N") != std::string::npos)
    format.replace(format.find("%N"), 2, to_stringf(nanosec(), "%09lld"));

  std::tm tm;
  memset(&tm, 0x00, sizeof(std::tm));

  tm.tm_hour = hour_;
  tm.tm_min  = min_;
  tm.tm_sec  = sec_;

  std::ostringstream str_time;
  str_time << std::put_time(&tm, format.c_str());
  return str_time.str();
}

inline SysTime
SysTime::now(const int64_t &adjust_sec)
{
  std::chrono::system_clock::time_point clock = std::chrono::system_clock::now();

  time_t time_t_v = std::chrono::system_clock::to_time_t(clock) + adjust_sec;

  std::tm to = localtime(time_t_v);

  int64_t nano =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          clock.time_since_epoch()
          ).count() - (int64_t)time_t_v * 1000000000LL;

  return SysTime(to.tm_hour, to.tm_min, to.tm_sec, nano);
}

inline SysTime &
SysTime::set(const int64_t &hour, const int64_t &min, const int64_t &sec, const int64_t &nano)
{
  std::tm from;
  memset(&from, 0x00, sizeof(std::tm));

  from.tm_hour = hour;
  from.tm_min  = min;
  from.tm_sec  = sec;

  time_t from_time_t = std::mktime(&from);

  if (nano >= 1000000000LL)
  {
    from_time_t += (nano / 1000000000LL);
    nano_        = nano % 1000000000LL;
  }
  else if (nano < 0)
  {
    int64_t share = (-nano / 1000000000LL);
    if ((-nano % 1000000000LL) > 0)
      ++share;

    from_time_t  -= share;
    nano_         = (share * 1000000000LL) - -nano;
  }
  else
  {
    nano_ = nano;
  }

  std::tm to = localtime(from_time_t);

  hour_ = to.tm_hour;
  min_  = to.tm_min;
  sec_  = to.tm_sec;

  return *this;
}

inline SysTime &
SysTime::operator=(const time_t &rhs)
{
  std::tm to = localtime(rhs);
  hour_ = to.tm_hour;
  min_  = to.tm_min;
  sec_  = to.tm_sec;
  nano_ = 0;

  return *this;
}

inline bool
SysTime::operator==(const SysTime &rhs) const
{
  return hour_ == rhs.hour_ && min_ == rhs.min_ && sec_ == rhs.sec_ && nano_ == rhs.nano_;
}

inline bool
SysTime::operator!=(const SysTime &rhs) const
{
  return !(*this == rhs);
}

inline bool
SysTime::operator<=(const SysTime &rhs) const
{
  if (hour_ == rhs.hour_ && min_ == rhs.min_ && sec_ == rhs.sec_) return nano_ <= rhs.nano_;
  if (hour_ == rhs.hour_ && min_ == rhs.min_) return sec_ <= rhs.sec_;
  if (hour_ == rhs.hour_) return min_ <= rhs.min_;
  return hour_ <= rhs.hour_;
}

inline bool
SysTime::operator>=(const SysTime &rhs) const
{
  if (hour_ == rhs.hour_ && min_ == rhs.min_ && sec_ == rhs.sec_) return nano_ >= rhs.nano_;
  if (hour_ == rhs.hour_ && min_ == rhs.min_) return sec_ >= rhs.sec_;
  if (hour_ == rhs.hour_) return min_ >= rhs.min_;
  return hour_ >= rhs.hour_;
}

inline bool
SysTime::operator< (const SysTime &rhs) const
{
  if (hour_ == rhs.hour_ && min_ == rhs.min_ && sec_ == rhs.sec_) return nano_ < rhs.nano_;
  if (hour_ == rhs.hour_ && min_ == rhs.min_) return sec_ < rhs.sec_;
  if (hour_ == rhs.hour_) return min_ < rhs.min_;
  return hour_ < rhs.hour_;
}

inline bool
SysTime::operator> (const SysTime &rhs) const
{
  if (hour_ == rhs.hour_ && min_ == rhs.min_ && sec_ == rhs.sec_) return nano_ > rhs.nano_;
  if (hour_ == rhs.hour_ && min_ == rhs.min_) return sec_ > rhs.sec_;
  if (hour_ == rhs.hour_) return min_ > rhs.min_;
  return hour_ > rhs.hour_;
}
