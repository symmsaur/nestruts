#pragma once

#include "fmt/core.h"
#include "fmt/printf.h"
#include <utility>

enum class log_level { trace, debug, instr, info, error };

inline log_level current_log_level = log_level::debug;

template <typename... Args>
void log(log_level level, fmt::format_string<Args...> format, Args &&...args) {
    if (level < current_log_level) {
        return;
    }
    fmt::print(format, std::forward<Args>(args)...);
}

template <typename... Args> void logf(log_level level, Args... args) {
    if (level < current_log_level) {
        return;
    }
    fmt::printf(args...);
}
