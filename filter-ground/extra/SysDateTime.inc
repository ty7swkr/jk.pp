#pragma once

#include <iomanip>
#include <sstream>
#include <string.h>
#include <time.h>

inline SysTime
SysDateTime::time() const
{
  return SysTime(hour(), min(), sec(), nanosec());
}

inline SysDate
SysDateTime::date() const
{
  return SysDate(year(), month(), day());
}

inline std::tm
SysDateTime::to_tm() const
{
  return SysTime::localtime(to_time_t());
}

inline struct timespec
SysDateTime::to_timespec() const
{
  return { to_time_t(), (long int)nanosec() };
}

inline struct timeval
SysDateTime::to_timeval() const
{
  return { to_time_t(), (long int)microsec() };
}

inline SysDateTime
SysDateTime::first_day_of_month() const
{
  SysDateTime st = *this;
  return st.day(1);
}

inline SysDateTime
SysDateTime::last_day_of_month() const
{
#define SystemTime_LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))

  static const int64_t ytab[2][12] =
  {
   { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
   { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
  };

  SysDateTime st = *this;

  std::time_t time_t_v  = st.to_time_t();
  std::tm     tm_value  = st.to_tm();

  time_t_v += (ytab[SystemTime_LEAPYEAR(tm_value.tm_year)][tm_value.tm_mon] - tm_value.tm_mday) * 86400;

  st.clock_ = nano_time_t_to_time_point(time_t_v, nanosec());

  return st;
}

inline SysDateTime
SysDateTime::first_time_of_day() const
{
  SysDateTime st = *this;
  return st.time(0, 0, 0, SysTime::Nanosec(0));
}

inline SysDateTime
SysDateTime::first_time_of_month() const
{
  SysDateTime st = *this;
  st.day(1);
  return st.time(0, 0, 0, SysTime::Nanosec(0));
}

inline SysDateTime
SysDateTime::last_time_of_day() const
{
  SysDateTime st = *this;
  return st.time(23, 59, 59, SysTime::Nanosec(NANOSEC-1));
}

inline SysDateTime
SysDateTime::last_time_of_month() const
{
  return last_day_of_month().last_time_of_day();
}

inline time_t
SysDateTime::to_time_t() const
{
  return std::chrono::system_clock::to_time_t(clock_);
}

inline std::string
SysDateTime::strftime(std::string format) const
{
  if (format.find("%L") != std::string::npos)
    format.replace(format.find("%L"), 2, to_stringf(millisec(), "%03lld"));

  if (format.find("%K") != std::string::npos)
    format.replace(format.find("%K"), 2, to_stringf(microsec(), "%06lld"));

  if (format.find("%N") != std::string::npos)
    format.replace(format.find("%N"), 2, to_stringf(nanosec(), "%09lld"));

  std::tm tm = SysTime::localtime(to_time_t());

  std::ostringstream str_time;
  str_time << std::put_time(&tm, format.c_str());
  return str_time.str();
}

inline bool
SysDateTime::strptime(const std::string &time_string,
                      const std::string &format)
{
  std::tm tm;
  memset(&tm, 0x00, sizeof(tm));

  std::istringstream ss(time_string);
  ss >> std::get_time(&tm, format.c_str());

  if (ss.fail() == true)
    return false;

  clock_ = time_t_to_time_point(std::mktime(&tm));
  null_ = false;
  return true;
}

inline SysDateTime
SysDateTime::from_string(const std::string &str, const std::string &format)
{
  SysDateTime datetime;
  datetime.strptime(str, format);
  return datetime;
}

inline SysDateTime
SysDateTime::from_string_compact(const std::string &str, const std::string &format)
{
//  "%Y-%m-%d"      <- format
//  "2024-03-22"    <- str
//   0123-67-90     (실제 인덱스)
//
//  1. 시작: str_index=1, index=1 (둘 다 'Y' 위치)
//  2. "2024" 파싱 후 str_index += 2 → str_index=3
//  3. 이 시점에서:
//     - index는 아직 'Y' 위치(1)
//     - str_index는 3
//  4. for문의 ++str_index, ++index 실행되면:
//     - index = 2 ('-' 위치)
//     - str_index = 4 ('0' 위치)

  int64_t year  = 1900;
  int64_t month = 1;
  int64_t day   = 1;
  int64_t hour  = 0;
  int64_t min   = 0;
  int64_t sec   = 0;
  int64_t nano  = 0;

  size_t str_index = 1;
  for (size_t index = 1; index < format.size(); ++index, ++str_index)
  {
    if (format.at(index-1) != '%')
      continue;

    if (str_index > str.length()-1)
      return SysDateTime();

    switch (format.at(index))
    {
      case 'y':
      {
        if ((str.length() - (str_index-1)) < 2)
          return SysDateTime();

        year = 2000 + std::atoll(str.substr(str_index-1, 2).c_str());
        break;
      }
      case 'Y':
      {
        if ((str.length() - (str_index-1)) < 3)
          return SysDateTime();

        year = std::atoll(str.substr(str_index-1, 4).c_str());
        str_index += 2;
        break;
      }
      case 'm':
      {
        if ((str.length() - (str_index-1)) < 2)
          return SysDateTime();

        month = std::atoll(str.substr(str_index-1, 2).c_str());
        break;
      }
      case 'd':
      {
        if ((str.length() - (str_index-1)) < 2)
          return SysDateTime();

        day = std::atoll(str.substr(str_index-1, 2).c_str());
        break;
      }
      case 'H':
      {
        if ((str.length() - (str_index-1)) < 2)
          return SysDateTime();

        hour = std::atoll(str.substr(str_index-1, 2).c_str());
        break;
      }
      case 'M':
      {
        if ((str.length() - (str_index-1)) < 2)
          return SysDateTime();

        min = std::atoll(str.substr(str_index-1, 2).c_str());
        break;
      }
      case 'S':
      {
        if ((str.length() - (str_index-1)) < 2)
          return SysDateTime();

        sec = std::atoll(str.substr(str_index-1, 2).c_str());
        break;
      }
      case 'L':
      {
        if ((str.length() - (str_index-1)) < 2)
          return SysDateTime();

        nano = std::atoll(str.substr(str_index-1, 3).c_str()) * MICROSEC;
        ++str_index;
        break;
      }
      case 'K':
      {
        if ((str.length() - (str_index-1)) < 6)
          return SysDateTime();

        nano = std::atoll(str.substr(str_index-1, 6).c_str()) * MILLISEC;
        str_index += 4;
        break;
      }
      case 'N':
      {
        if ((str.length() - (str_index-1)) < 9)
          return SysDateTime();

        nano = std::atoll(str.substr(str_index-1, 9).c_str());
        str_index += 7;
        break;
      }
      default: break;
    }
  }

  return SysDateTime(SysDate(year, month, day), SysTime(hour, min, sec, nano));
}

inline SysDateTime &
SysDateTime::current(const int64_t &adjust_sec)
{
  clock_ = std::chrono::system_clock::now();
  null_  = false;

  *this += SysTime::Sec(adjust_sec);

  return *this;
}

inline int64_t
SysDateTime::year() const
{
  return to_tm().tm_year + 1900;
}

inline int64_t
SysDateTime::month() const
{
  return to_tm().tm_mon + 1;
}

inline int64_t
SysDateTime::day() const
{
  return to_tm().tm_mday;
}

inline int64_t
SysDateTime::hour() const
{
  return to_tm().tm_hour;
}

inline int64_t
SysDateTime::min() const
{
  return to_tm().tm_min;
}

inline int64_t
SysDateTime::sec() const
{
  return to_tm().tm_sec;
}

inline int64_t
SysDateTime::millisec() const
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(clock_.time_since_epoch()).count() - (to_time_t() * MILLISEC);
}

inline int64_t
SysDateTime::microsec() const
{
  return std::chrono::duration_cast<std::chrono::microseconds>(clock_.time_since_epoch()).count() - (to_time_t() * MICROSEC);
}

inline int64_t
SysDateTime::nanosec() const
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(clock_.time_since_epoch()).count() - (to_time_t() * NANOSEC);
}

inline SysDateTime &
SysDateTime::date(const int64_t &year, const int64_t &month, const int64_t &day)
{
  std::tm value = to_tm();

  value.tm_year = year  - 1900;
  value.tm_mon  = month - 1;
  value.tm_mday = day;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nanosec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::time(const int64_t &hour, const int64_t &min, const int64_t &sec, const SysTime::Millisec &msec)
{
  return time(hour, min, sec, SysTime::Nanosec(msec() * 1000000LL));
}

inline SysDateTime &
SysDateTime::time(const int64_t &hour, const int64_t &min, const int64_t &sec, const SysTime::Microsec &usec)
{
  return time(hour, min, sec, SysTime::Nanosec(usec() * 1000LL));
}

inline SysDateTime &
SysDateTime::time(const int64_t &hour, const int64_t &min, const int64_t &sec, const SysTime::Nanosec &nsec)
{
  std::tm value = SysTime::localtime(to_time_t());

  value.tm_hour = hour;
  value.tm_min  = min;
  value.tm_sec  = sec;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nsec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::datetime(const int64_t &year, const int64_t &month, const int64_t &day,
                          const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Millisec &msec)
{
  return datetime(year, month, day, hour, min, sec, SysTime::Nanosec(msec() * 1000000LL));
}

inline SysDateTime &
SysDateTime::datetime(const int64_t &year, const int64_t &month, const int64_t &day,
                          const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Microsec &usec)
{
  return datetime(year, month, day, hour, min, sec, SysTime::Nanosec(usec() * 1000LL));
}

inline SysDateTime &
SysDateTime::datetime(const int64_t &year, const int64_t &month, const int64_t &day,
                          const int64_t &hour, const int64_t &min,   const int64_t &sec, const SysTime::Nanosec &nsec)
{
  std::tm value = to_tm();

  value.tm_year = year  - 1900;
  value.tm_mon  = month - 1;
  value.tm_mday = day;
  value.tm_hour = hour;
  value.tm_min  = min;
  value.tm_sec  = sec;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nsec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::year(const int64_t &year)
{
  std::tm value = SysTime::localtime(to_time_t());

  value.tm_year = year - 1900;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nanosec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::month(const int64_t &month)
{
  std::tm value = SysTime::localtime(to_time_t());

  value.tm_mon = month - 1;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nanosec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::day(const int64_t &day)
{
  std::tm value = SysTime::localtime(to_time_t());

  value.tm_mday = day;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nanosec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::hour(const int64_t &hour)
{
  std::tm value = SysTime::localtime(to_time_t());

  value.tm_hour = hour;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nanosec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::min(const int64_t &min)
{
  std::tm value = SysTime::localtime(to_time_t());

  value.tm_min = min;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nanosec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::sec(const int64_t &sec)
{
  std::tm value = SysTime::localtime(to_time_t());

  value.tm_sec = sec;

  clock_ = nano_time_t_to_time_point(std::mktime(&value), nanosec());
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::millisec(const int64_t &millisec)
{
  clock_ = milli_time_t_to_time_point(to_time_t(), millisec);
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::microsec(const int64_t &microsec)
{
  clock_ = micro_time_t_to_time_point(to_time_t(), microsec);
  null_ = false;

  return *this;
}

inline SysDateTime &
SysDateTime::nanosec(const int64_t &nanosec)
{
  clock_ = nano_time_t_to_time_point(to_time_t(), nanosec);
  null_ = false;

  return *this;
}

inline int64_t
SysDateTime::day_of_week() const
{
  return to_tm().tm_wday;
}

