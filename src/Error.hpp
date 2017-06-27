/*
 * RCMC -- Model Checking for C11 programs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Author: Michalis Kokologiannakis <mixaskok@gmail.com>
 */

#ifndef __ERROR_HPP__
#define __ERROR_HPP__

#include <llvm/Support/Debug.h>
#include <llvm/Support/Format.h>
#include <set>
#include <string>

#define WARN(msg) Error::warn() << (msg)
#define WARN_ONCE(id, msg) Error::warnOnce(id) << (msg)
#define WARN_ON(condition, msg) Error::warnOn(condition) << (msg)
#define WARN_ON_ONCE(condition, id, msg) Error::warnOnOnce(condition, id) << (msg)

#define BUG() do { \
	llvm::errs() << "BUG: Failure at " << __FILE__ ":" << __LINE__ \
		     << "/" << __func__ << "()!\n";		       \
	abort();						       \
	} while (0)
#define BUG_ON(condition) do { if (condition) BUG(); } while (0)

/* TODO: Replace these codes with enum? */
#define ECOMPILE 1
#define EVERFAIL 42

namespace Error {

	llvm::raw_ostream &warn();
	llvm::raw_ostream &warnOnce(const std::string &warningID);
	llvm::raw_ostream &warnOn(bool condition);
	llvm::raw_ostream &warnOnOnce(bool condition, const std::string &warningID);

}

#endif /* __ERROR_HPP__ */
