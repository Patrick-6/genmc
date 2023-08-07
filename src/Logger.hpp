/*
 * GenMC -- Generic Model Checking.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <iostream>
#include <sstream>

enum class LogLevel {
	Quiet,
	Error,
	Warning,
	Tip,
#ifdef ENABLE_GENMC_DEBUG
	Debug1,
	Debug2,
	Debug3,
	Debug4,
#endif
};

std::ostream& operator<<(std::ostream& rhs, const LogLevel l);

class Logger {

public:
	Logger(LogLevel l = LogLevel::Warning) {
		buffer_ << l << ": ";
	}

	template <typename T>
	Logger &operator<<(const T &msg) {
		buffer_ << msg;
		return *this;
	}

	~Logger() {
		/*
		 * 1. We don't have to flush --- this is stderr
		 * 2. Stream ops are atomic according to POSIX:
		 *    http://www.gnu.org/s/libc/manual/html_node/Streams-and-Threads.html
		 */
		std::cerr << buffer_.str();
	}

private:
	std::ostringstream buffer_;
};

static inline LogLevel logLevel = LogLevel::Tip;

#define LOG(level)				\
	if (level < logLevel) ;			\
	else Logger(level)

#endif /* __LOGGER_HPP__ */
