#pragma once
#include <string>
#include <map>
#include <initializer_list>
#include <utility>

namespace util {

enum class LogLevel { Debug, Info, Warn, Error };

void log(LogLevel level, const std::string& msg,
         const std::map<std::string,std::string>& kv = {});

// Helper overloads to support both 2-arg (level, msg) and
// 3-arg (level, msg, kv) without GNU-specific macro behavior.
namespace detail {
  inline void log_helper(LogLevel level, const std::string& msg) {
    ::util::log(level, msg);
  }

  inline void log_helper(LogLevel level, const std::string& msg,
                         const std::map<std::string, std::string>& kv) {
    ::util::log(level, msg, kv);
  }

  inline void log_helper(LogLevel level, const std::string& msg,
                         std::initializer_list<std::pair<const std::string, std::string>> kv_list) {
    ::util::log(level, msg, std::map<std::string, std::string>(kv_list));
  }
} // namespace detail

} // namespace util

#define LOG_DEBUG(...) ::util::detail::log_helper(::util::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  ::util::detail::log_helper(::util::LogLevel::Info,  __VA_ARGS__)
#define LOG_WARN(...)  ::util::detail::log_helper(::util::LogLevel::Warn,  __VA_ARGS__)
#define LOG_ERROR(...) ::util::detail::log_helper(::util::LogLevel::Error, __VA_ARGS__)
