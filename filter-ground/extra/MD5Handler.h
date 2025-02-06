/*
 * MD5Handler.h
 *
 *  Created on: 2024. 12. 10.
 *      Author: tys
 */
#pragma once

#include <openssl/evp.h>
#include <string>
#include <vector>
#include <cstring>  // memcpy 사용을 위해

//int main()
//{
//  // 기본적인 문자열 해싱
//  {
//    MD5Handler md5;
//    md5.update("Hello, World!");
//    MD5Result result = md5.final();
//    std::cout << "String hash: " << result.first << ", " << result.second << std::endl;
//  }
//
//  // 여러 데이터를 순차적으로 해싱
//  {
//    MD5Handler md5;
//    md5.update("First part");
//    md5.update("Second part");
//    md5.update("Third part");
//    MD5Result result = md5.final();
//    std::cout << "Combined hash: " << result.first << ", " << result.second << std::endl;
//  }
//
//  // std::vector 해싱
//  {
//    std::vector<int> numbers = {1, 2, 3, 4, 5};
//    MD5Handler md5;
//    md5.update(numbers);
//    MD5Result result = md5.final();
//    std::cout << "Vector hash: " << result.first << ", " << result.second << std::endl;
//  }
//
//  // 배열 해싱
//  {
//    int arr[] = {10, 20, 30, 40, 50};
//    MD5Handler md5;
//    md5.update(arr);
//    MD5Result result = md5.final();
//    std::cout << "Array hash: " << result.first << ", " << result.second << std::endl;
//  }
//
//  // 복합 데이터 해싱
//  {
//    MD5Handler md5;
//    std::string header = "Header";
//    int numbers[] = {1, 2, 3};
//    std::vector<char> data = {'a', 'b', 'c'};
//
//    md5.update(header);
//    md5.update(numbers);
//    md5.update(data);
//
//    MD5Result result = md5.final();
//    std::cout << "Complex hash: " << result.first << ", " << result.second << std::endl;
//  }
//
//  // 결과 비교
//  {
//    MD5Handler md5_1, md5_2;
//    md5_1.update("Same content");
//    md5_2.update("Same content");
//
//    MD5Result result1 = md5_1.final();
//    MD5Result result2 = md5_2.final();
//
//    if (result1 == result2) {
//      std::cout << "Hashes are equal" << std::endl;
//    }
//  }
//
//  return 0;
//}

// openssl 3.0 기준
struct MD5Result
{
  MD5Result() {}
  MD5Result(const MD5Result &) = default;

  int64_t first   = 0;
  int64_t second  = 0;

  bool operator==(const MD5Result &lhs) const { return first == lhs.first && second == lhs.second; }
  bool operator!=(const MD5Result &lhs) const { return first != lhs.first || second != lhs.second; }

  MD5Result &operator=(const MD5Result &lhs)
  {
    first  = lhs.first;
    second = lhs.second;
    return *this;
  }
};

class MD5Handler
{
public:
  MD5Handler()
  {
    mdctx_ = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx_, EVP_md5(), NULL);
  }

  ~MD5Handler()
  {
    EVP_MD_CTX_free(mdctx_);
  }

  void update(const void *data, size_t size)
  {
    EVP_DigestUpdate(mdctx_, data, size);
  }

  void update(const std::string& str)
  {
    EVP_DigestUpdate(mdctx_, str.c_str(), str.length());
  }
  template<typename T>
  void update(const T &value)
  {
    auto str = std::to_string(value);
    EVP_DigestUpdate(mdctx_, str.data(), str.size());
  }

  template<size_t N>
  void update(const char (&value)[N])
  {
    EVP_DigestUpdate(mdctx_, value, N-1);
  }

  template<typename T, size_t N>
  void update(const T (&value)[N])
  {
    EVP_DigestUpdate(mdctx_, value, sizeof(T) * N);
  }

  template<typename T>
  void update(const std::vector<T> &value)
  {
    EVP_DigestUpdate(mdctx_, value.data(), value.size());
  }

  MD5Result final()
  {
#define MD5_DIGEST_LENGTH 16
    unsigned char result[MD5_DIGEST_LENGTH];
    unsigned int length;
    EVP_DigestFinal_ex(mdctx_, result, &length);

    MD5Result hash_result;
    // 첫 8바이트와 다음 8바이트를 각각 64비트 정수로 변환
    memcpy(&hash_result.first,  result,   8);
    memcpy(&hash_result.second, result+8, 8);

    return hash_result;
  }

private:
  EVP_MD_CTX *mdctx_;
};

