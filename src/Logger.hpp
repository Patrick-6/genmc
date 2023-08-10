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

#include "Verbosity.hpp"
#include <llvm/Support/raw_ostream.h>
#include <set>

class Logger {

public:
	Logger(VerbosityLevel l = VerbosityLevel::Warning) : buffer_(str_) {
		buffer_ << l;
	}

	template<typename T>
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
		llvm::errs() << buffer_.str();
	}

protected:
	std::string str_;
	llvm::raw_string_ostream buffer_;
};

/* A logger that logs each message only once */
class LoggerOnce : public Logger {

public:
	LoggerOnce(const std::string &id, VerbosityLevel l = VerbosityLevel::Warning)
		: Logger(l), id(id) {}

	template<typename T>
	LoggerOnce &operator<<(const T &msg) {
		if (ids.count(id)) {
			this->str_.clear();
			return *this;
		}
		ids.insert(id);
		Logger::operator<<(msg);
		return *this;
	}

private:
	const std::string &id;
	static thread_local inline std::set<std::string> ids;
};

static inline VerbosityLevel logLevel = VerbosityLevel::Tip;

#define LOG(level)				\
	if (level > logLevel) ;			\
	else Logger(level)

#define LOG_ONCE(id, level)			\
	if (level > logLevel) ;			\
	else LoggerOnce(id, level)

#endif /* __LOGGER_HPP__ */
