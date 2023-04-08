#pragma once

#include "fmt/printf.h"

enum class log_level {
	debug,
	instr,
	info,
	error
};

inline log_level current_log_level = log_level::debug;

template<typename... Args>
void logf(log_level level, Args... args)
{
	if (level < current_log_level)
	{
		return;
	}
	fmt::printf(args...);
}
