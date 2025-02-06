/*
 * assist.h
 *
 *  Created on: 2024. 10. 23.
 *      Author: tys
 */

#pragma once

#include <string>
#include <iomanip> // std::setw를 사용하기 위해 필요
#include <cctype>
#include <climits> // 또는 #include <limits.h>
#include <cstring> // strnlen
#include <algorithm>
#include <sstream>
#include <vector>
#include <array>
#include <string>
#include <functional>

#include <csignal>
#include <arpa/inet.h> // ntohl...

// (void)x 는 컴파일타임에 체크되도록 함.
#define VARNAME(x) ((void)x, std::string(#x))

// std::string용 런타임 해시
inline uint64_t
string_hash(const std::string &str)
{
  const char *p = str.c_str();
  return *p ? static_cast<uint64_t>(*p) + 33 * string_hash(p + 1) : 5381;
}

// 컴파일타임 해시
inline constexpr uint64_t
constexpr_hash(const char *p)
{
  return *p ? static_cast<uint64_t>(*p) + 33 * constexpr_hash(p + 1) : 5381;
}

constexpr size_t
operator"" _hash(const char* str)
{
  return constexpr_hash(str);
}

template<typename IT, typename Comp>
IT std14_min_element(IT begin, IT end, Comp comp)
{
  IT min = begin;
  for (IT it = std::next(begin); it != end; ++it)
    if (comp(*it, *min) == true)
      min = it;
  return min;
}

template<size_t N> std::string
to_string(const char (&value)[N])
{
  std::string str;
  for (size_t index = 0; index < N+1; ++index)
  {
    if (value[index] == '\0')
      break;

    str.append(1, value[index]);
  }

  return str;
}

template<size_t N> std::string
to_string(const std::array<char, N>& arr)
{
  std::string str; str.reserve(arr.size());
  for (size_t index = 0; index < arr.size(); ++index)
  {
    if (arr[index] == '\0')
      break;

    str.push_back(arr.at(index));
  }

  return str;
}

template<int sig_num> void
lambda_signal_handler(std::function<void(int)>&& handler);

template<int sig_num> void
lambda_signal_handler(std::function<void(   )>&& handler);

inline std::string
ltrim(const std::string &str, const std::string chars = " ")
{
  std::string::size_type const first = str.find_first_not_of(chars);
  return (first == std::string::npos) ? std::string() : std::string(str.substr(first));
}

inline std::string
rtrim(const std::string &str, const std::string chars = " ")
{
  std::string::size_type const last = str.find_last_not_of(chars);
  return (last == std::string::npos) ? std::string() : std::string(str.substr(0, last+1));
}

inline std::string
trimmed(const std::string &str, const std::string &chars = " ")
{
  std::string::size_type const first = str.find_first_not_of(chars);
  return (first == std::string::npos) ? std::string() : std::string(str.substr(first, str.find_last_not_of(chars)-first+1));
}

inline std::string
extract(const std::string &str,
        const std::string &sta_str,
        const std::string &end_str,
        const bool &include_sta_end_str = true)
{
  std::string::size_type pos_sta = str.find(sta_str);
  if (pos_sta == std::string::npos)
    return {};

  pos_sta += sta_str.length();
  if (end_str.empty() == true)
    return str.substr(pos_sta);

  std::string::size_type pos_end = str.find(end_str, pos_sta);
  if (pos_end == std::string::npos)
    return {};

  if (include_sta_end_str == false)
    return str.substr(pos_sta+sta_str.size(), (pos_end - pos_sta)-end_str.size());

  return str.substr(pos_sta, pos_end - pos_sta);
}

inline std::vector<std::string>
split(std::string str, const std::string &delimiter)
{
  std::vector<std::string> result;
  size_t pos = 0;
  std::string token;
  while ((pos = str.find(delimiter)) != std::string::npos)
  {
    token = str.substr(0, pos);
    result.push_back(token);
    str.erase(0, pos + delimiter.length());
  }
  result.push_back(str);
  return result;
}

inline std::string
to_upper(const std::string &str)
{
  std::string returnValue;
  for (const auto &c : str)
    returnValue += toupper(c);

  return returnValue;
}

inline std::string
to_lower(const std::string &str)
{
  std::string returnValue;
  for (const auto &c : str)
    returnValue += tolower(c);

  return returnValue;
}

template<typename T> T
to_number(const std::string &in, std::ios_base &(*base)(std::ios_base &) = std::dec)
{
  T out;
  std::stringstream ss;
  ss << base << in;
  ss >> out;
  return out;
}

inline std::string
string_errno(int err_no = errno)
{
  char buff[256];
  return strerror_r(err_no, buff, sizeof(buff));
}

// safe한 substr
#include <string>
inline std::string
substr(const std::string &str, const std::string &sta_str, size_t length = 0)
{
  if (str.length() < sta_str.length())
    return {};

  std::string::size_type pos_sta = str.find(sta_str);
  if (pos_sta == std::string::npos)
    return {};

  length += sta_str.length();

  if (length == 0)
    length = str.length() - pos_sta;
  else if (length > (str.length() - pos_sta))
    length = str.length() - pos_sta;

  return str.substr(pos_sta, length);
}

/// @brief str에서 index부터 length만큼 잘라낸다.<br>
/// 일반 std::string의 멤버함수 substr은 index가 문자열 총 길이를 넘는 경우 throw를 반환한다.<br>
/// 이 함수는 throw 하지 않고 index가 넘지 않는 부분까지 처리하여 준다.
/// index가 음수이면 length는 무시된다.
inline std::string
substr(const std::string &str, int index, size_t length = 0)
{
  if (index < 0)
  {
    if (str.length() == 0)
      return {};

    if ((int)str.length() < -index)
      index = -((int)str.length());

    int pos = str.length() - (-index);
    index = str.length() - pos;

    return str.substr(pos, index);
  }

  if (length == 0)
    length = str.length();

  if (index > (int)str.length()-1)
    return {};

  if (index + length > str.length())
    length = str.length() - index;

  return str.substr(index, length);
}


inline bool
compare_front(const std::string &src, const std::string &tgt)
{
  if (src.length() < tgt.length())
    return false;

  return src.compare(0, tgt.length(), tgt) == 0;
}

inline bool
compare_front(const std::string &src, const std::string &tgt, const size_t &size)
{
  return compare_front(substr(src, 0, size), tgt);
}

inline bool
compare_rear(const std::string &src, const std::string &tgt)
{
  if (src.length() < tgt.length())
    return false;

  return src.compare(src.length()-tgt.length(), tgt.length(), tgt) == 0;
}

inline std::string
replace_all(const std::string &str, const std::string &pattern, const std::string &replace)
{
  std::string result = str;
  std::string::size_type pos = 0;
  std::string::size_type offset = 0;

  while((pos = result.find(pattern, offset)) != std::string::npos)
  {
    result.replace(result.begin() + pos, result.begin() + pos + pattern.size(), replace);
    offset = pos + replace.size();
  }

  return result;
}

inline std::string
replace_all(const std::string &str, const std::initializer_list<std::pair<std::string, std::string>> &replaces)
{
  std::string replaced = str;
  for (const auto &replace : replaces)
    replaced = replace_all(replaced, replace.first, replace.second);

  return replaced;
}

template<typename T> std::string
to_hex_string(const T &value, const bool &upper = true)
{
  std::stringstream ss;
  // sizeof(T) * 2을 통해 16진수 자리수 계산 (예: uint16_t -> 4, uint32_t -> 8)
  if (upper == true)
    ss << "0x" << std::hex << std::uppercase << std::setw(sizeof(T) * 2) << std::setfill('0') << value;
  else
    ss << "0x" << std::hex << std::setw(sizeof(T) * 2) << std::setfill('0') << value;

  return ss.str();
}

template<typename T> std::string
to_stringf(const T &target, const std::string &format)
{
  int size_f = std::snprintf(nullptr, 0, format.c_str(), target) + 1;
  if (size_f <= 0) return "";

  char *buff = (char *)calloc(size_f, sizeof(char));
  std::snprintf(buff, size_f, format.c_str(), target);

  std::string ret_buff = buff;
  free(buff);

  return ret_buff;
}

/**
 * 시스템이 Big Endian인지 검사
 * @return bool
 */
inline bool
is_big_endian()
{
  const unsigned char int_bytes[sizeof(int)] = {0xFF};
  const int *int_0xFF = reinterpret_cast<const int *>(int_bytes);
  const int  msb_0xFF = 0xFF << (sizeof(int) - 1) * CHAR_BIT;

  return (*int_0xFF & msb_0xFF) != 0;
}

/**
 * @param host값
 * @return 네트워크값으로 변경된 값
 */
inline uint64_t
htonll(const uint64_t &host)
{
  if (is_big_endian() == true)
    return host;

  union { uint32_t lv[2]; uint64_t llv; } u;
  u.lv[0] = htonl(host >> 32);
  u.lv[1] = htonl(host & 0xFFFFFFFFULL);
  return u.llv;
}

/**
 * @param network
 * @return 네트워크값으로 변경된 값
 */
inline uint64_t
ntohll(const uint64_t &network)
{
  if (is_big_endian() == true)
    return network;

  union { uint32_t lv[2]; uint64_t llv; } u;
  u.llv = network;
  return ((uint64_t)ntohl(u.lv[0]) << 32) | (uint64_t)ntohl(u.lv[1]);
}

template<typename T> T
endian_swap(const T &target)
{
  T data = target;
  unsigned char *raw = reinterpret_cast<unsigned char*>(&data);
  std::reverse(raw, raw + sizeof(T));

  return data;
}

inline double
htond(const double &host)
{
  if (is_big_endian() == true)
    return host;

  return endian_swap<double>(host);
}

inline double
ntohd(const double &network)
{
  if (is_big_endian() == true)
    return network;

  return endian_swap<double>(network);
}

/// @brief 바이트 오더링 int8_t  host to network
inline int16_t  host_to_network(const int8_t    &host)    { return host; }
/// @brief 바이트 오더링 int16_t host to network
inline int16_t  host_to_network(const int16_t   &host)    { return htons (host); }
/// @brief 바이트 오더링 int32_t host to network
inline int32_t  host_to_network(const int32_t   &host)    { return htonl (host); }
/// @brief 바이트 오더링 int64_t host to network
inline int64_t  host_to_network(const int64_t   &host)    { return htonll(host); }

/// @brief 바이트 오더링 uint16_t host to network
inline uint8_t  host_to_network(const uint8_t   &host)    { return host; }
/// @brief 바이트 오더링 uint16_t host to network
inline uint16_t host_to_network(const uint16_t  &host)    { return htons (host); }
/// @brief 바이트 오더링 uint32_t host to network
inline uint32_t host_to_network(const uint32_t  &host)    { return htonl (host); }
/// @brief 바이트 오더링 uint64_t host to network
inline uint64_t host_to_network(const uint64_t  &host)    { return htonll(host); }

/// @brief 바이트 오더링 int8_t  network to host
inline int16_t  network_to_host(const int8_t    &network) { return network; }
/// @brief 바이트 오더링 int16_t network to host
inline int16_t  network_to_host(const int16_t   &network) { return ntohs (network); }
/// @brief 바이트 오더링 int32_t network to host
inline int32_t  network_to_host(const int32_t   &network) { return ntohl (network); }
/// @brief 바이트 오더링 int64_t network to host
inline int64_t  network_to_host(const int64_t   &network) { return ntohll(network); }

/// @brief 바이트 오더링 uint8_t  network to host
inline uint16_t network_to_host(const uint8_t   &network) { return network; }
/// @brief 바이트 오더링 uint16_t network to host
inline uint16_t network_to_host(const uint16_t  &network) { return ntohs (network); }
/// @brief 바이트 오더링 uint32_t network to host
inline uint32_t network_to_host(const uint32_t  &network) { return ntohl (network); }
/// @brief 바이트 오더링 uint64_t network to host
inline uint64_t network_to_host(const uint64_t  &network) { return ntohll(network); }

/// @brief 바이트 오더링 double host to network
inline double   host_to_network(const double    &host)    { return htond (host); }
/// @brief 바이트 오더링 double network to host
inline double   network_to_host(const double    &network) { return ntohd (network); }

#define FILE_LINE_STR           std::string(__FILE__)  + ":" + std::to_string(__LINE__)
#define pair_error(c,name,str)  std::make_pair(c, name + " " + FILE_LINE_STR + std::string(str))

// container
template<typename B, typename E> std::string
concat_with_delimiter(const std::string &delim, B bit, E eit)
{
  std::string str;
  for (auto it = bit; it != eit; ++it)
    str += (str.empty() ? *it : str + delim + *it);

  return str;
}

// C++17 미만인 경우
template<typename ARG> void
concat_with_delimiter_stream(std::stringstream &stream, const ARG &arg)
{
  stream << arg;
}

template<> inline void
concat_with_delimiter_stream(std::stringstream &stream, const bool &arg)
{
  if (arg == true) stream << "true";
  else stream << "false";
}

// int8_t 타입 특수화
template<> inline void
concat_with_delimiter_stream(std::stringstream &stream, const int8_t &arg)
{
  stream << static_cast<int32_t>(arg); // int로 변환하여 출력
}

// unt8_t 타입 특수화
template<> inline void
concat_with_delimiter_stream(std::stringstream &stream, const uint8_t &arg)
{
  stream << static_cast<uint32_t>(arg); // int로 변환하여 출력
}

template<typename ARG> void
concat_with_delimiter(size_t &index, std::stringstream &stream, const std::string &delim, const ARG &arg)
{
  if (index > 0) stream << delim;
  concat_with_delimiter_stream(stream, arg);
}

template<typename FIRST, typename... ARGS> void
concat_with_delimiter(size_t &index, std::stringstream &stream, const std::string &delim, const FIRST &first, const ARGS &... args)
{
  if (index > 0) stream << delim;
//  stream << first;
  concat_with_delimiter_stream(stream, first);
  concat_with_delimiter(++index, stream, delim, args...);
}

template<typename... ARGS> std::string
concat_with_delimiter(const std::string &delim, const ARGS &... args)
{
  std::stringstream stream;
  size_t index = (size_t)-1;
  concat_with_delimiter(++index, stream, delim, args...);
  return stream.str();
}

template<int sig_num>
class __LambdaSignalHandlerHelper__
{
private:
  friend void lambda_signal_handler<sig_num>(std::function<void(int)>&& handler);
  friend void lambda_signal_handler<sig_num>(std::function<void(   )>&& handler);

  static void wrapper(int signal)
  {
    if (handler_fn1 != nullptr) handler_fn1(signal);
    if (handler_fn2 != nullptr) handler_fn2();
  }
  static std::function<void(int)> handler_fn1;
  static std::function<void(   )> handler_fn2;
};

template<int sig_num> std::function<void(int)> __LambdaSignalHandlerHelper__<sig_num>::handler_fn1 = nullptr;
template<int sig_num> std::function<void(   )> __LambdaSignalHandlerHelper__<sig_num>::handler_fn2 = nullptr;

template<int sig_num> void
lambda_signal_handler(std::function<void(int)>&& handler)
{
  __LambdaSignalHandlerHelper__<sig_num>::handler_fn2 = nullptr;
  __LambdaSignalHandlerHelper__<sig_num>::handler_fn1 = std::move(handler);
  std::signal(sig_num, __LambdaSignalHandlerHelper__<sig_num>::wrapper);
}

template<int sig_num> void
lambda_signal_handler(std::function<void()>&& handler)
{
  __LambdaSignalHandlerHelper__<sig_num>::handler_fn1 = nullptr;
  __LambdaSignalHandlerHelper__<sig_num>::handler_fn2 = std::move(handler);
  std::signal(sig_num, __LambdaSignalHandlerHelper__<sig_num>::wrapper);
}


