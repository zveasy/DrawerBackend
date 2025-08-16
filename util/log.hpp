#pragma once
#include <string>
#include <map>

namespace util {

enum class LogLevel { Debug, Info, Warn, Error };

void log(LogLevel level, const std::string& msg,
         const std::map<std::string,std::string>& kv = {});

} // namespace util

#define LOG_DEBUG(msg, ...) ::util::log(::util::LogLevel::Debug, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...)  ::util::log(::util::LogLevel::Info,  msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...)  ::util::log(::util::LogLevel::Warn,  msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...) ::util::log(::util::LogLevel::Error, msg, ##__VA_ARGS__)
