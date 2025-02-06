/*
 * Duration.h
 *
 *  Created on: 2024. 11. 24.
 *      Author: tys
 */

#pragma once

#include <extra/SysDateTime.h>
#include <chrono>
#include <sstream>
/**
 * @class SysDateTimeDiff
 * @brief 두 날짜시간 사이의 차이를 계산하고 표현하는 클래스
 *
 * @details 주요 기능:
 * - 두 시점 간의 시간 차이 계산
 * - 일/시간/분/초 단위의 차이 계산
 * - 밀리초/마이크로초/나노초 단위의 정밀한 차이 계산
 * - 시간 차이의 절대값 계산
 * - 개별 시간 단위(일,시,분,초)로 분해된 차이 계산
 * - 총 시간을 특정 단위로 환산한 결과
 *
 * @note 출력 예시: to_string() -> "0 days 01:30:13.000287481"
 * @see SysDateTime
 */

class SysDateTimeDiff;

SysDateTimeDiff operator-(const SysDateTime &small, const SysDateTime &big);

class SysDateTimeDiff
{
public:
  friend SysDateTimeDiff &abs(SysDateTimeDiff &&diff)
  {
    // 절대값 계산
    if (diff.components.time_point_ < std::chrono::system_clock::duration::zero())
      diff.components.time_point_ = -diff.components.time_point_;

    return diff;
  }

public:
  SysDateTimeDiff(const SysDateTime &big, const SysDateTime &small) : components(big, small) {}

  // 총 걸린 시간으로 환산.
  double  days      () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(components.time_point_).count() / (24.0 * 60 * 60 * 1000000000); }
  double  hours     () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(components.time_point_).count() / (60.0 * 60 * 1000000000); }
  double  minutes   () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(components.time_point_).count() / (60.0 * 1000000000); }
  double  seconds   () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(components.time_point_).count() / 1000000000.0; }
  double  millisecs () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(components.time_point_).count() / 1000000.0; }
  double  microsecs () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(components.time_point_).count() / 1000.0; }
  int64_t nanosecs  () const { return std::chrono::duration_cast<std::chrono::nanoseconds>(components.time_point_).count(); }

  // 총 걸린시간을 단위별로 1.55일 = days = 1, hours = 12, minutes = 30, seconds = 0
  class Components
  {
  public:
    Components(const SysDateTime &big, const SysDateTime &small) : time_point_(big.time_point() - small.time_point()) {}

    int64_t days()      const { return std::chrono::duration_cast<std::chrono::hours>  (time_point_).count() / 24; }
    int64_t hours()     const { return std::chrono::duration_cast<std::chrono::hours>  (time_point_).count() -  (days()*24); }
    int64_t minutes()   const { return std::chrono::duration_cast<std::chrono::minutes>(time_point_).count() - ((days()*24*60) + (hours()*60)); }
    int64_t seconds()   const { return std::chrono::duration_cast<std::chrono::seconds>(time_point_).count() - ((days()*24*60*60) + (hours()*60*60) + (minutes()*60)); }
    int64_t millisecs() const { return nanosecs() / 1000000; }
    int64_t microsecs() const { return nanosecs() / 1000; }
    int64_t nanosecs()  const { return std::chrono::duration_cast<std::chrono::nanoseconds>(time_point_).count() -
                                       ((days    ()*24*60*60*1000000000LL) +
                                        (hours   ()*60*60   *1000000000LL) +
                                        (minutes ()*60      *1000000000LL) +
                                        (seconds ()         *1000000000LL));}

    std::string to_string() const
    {
      std::ostringstream oss;
      oss << days() << " days "
          << std::setfill('0') << std::setw(2) << hours    () << ":"
          << std::setfill('0') << std::setw(2) << minutes  () << ":"
          << std::setfill('0') << std::setw(2) << seconds  () << "."
          << std::setfill('0') << std::setw(3) << millisecs() << " "
          << std::setfill('0') << std::setw(6) << microsecs() << " "
          << std::setfill('0') << std::setw(9) << nanosecs ();
      return oss.str();
    }

  private:
    std::chrono::system_clock::duration time_point_;
    friend       SysDateTimeDiff &abs(SysDateTimeDiff &&diff);
    friend class SysDateTimeDiff;
  };

  Components components;
};

inline SysDateTimeDiff
operator-(const SysDateTime &big, const SysDateTime &small)
{
  return SysDateTimeDiff(big, small);
}



