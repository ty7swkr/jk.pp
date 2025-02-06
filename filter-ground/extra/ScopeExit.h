/*
 * ScopeExit.h
 *
 *  Created on: 2022. 6. 29.
 *      Author: tys
 */

#pragma once

#include <functional>

#define MAKE_SCOPE_EXIT_VAR1(arg1, arg2, arg3)  MAKE_SCOPE_EXIT_VAR2(arg1, arg2(arg3))
#define MAKE_SCOPE_EXIT_VAR2(arg1, arg2)        arg1 ## arg2
#define SCOPE_EXIT(CODE)                        ScopeExit MAKE_SCOPE_EXIT_VAR1(scpoe_exit_, __LINE__, [&]()CODE)

/**
 * example1 ---------
 * SCOPE_EXIT({ std::cout << "end of block" << std::endl; });
 *
 * example2 ---------
 * ScopeExit sc([&](){....});
 * s.ignore();
 */

class ScopeExit
{
public:
  ScopeExit() = delete;
  ScopeExit(const ScopeExit &) = delete;

  explicit ScopeExit(const std::function<void()> &exit_func)
  : exit_func_(exit_func) {}

  ~ScopeExit()
  {
    if (ignore_ == true)
      return;

    if (exit_func_ == nullptr)
      return;

    exit_func_();
  }

  void ignore () { ignore_ = true; }

private:
  std::function<void()> exit_func_ = nullptr;
  bool ignore_  = false;
};
