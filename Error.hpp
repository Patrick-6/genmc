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

#include <assert.h>

#include <llvm/Support/Debug.h>

#define BUG_ON(condition) assert(!(condition))
#define WARN(msg) llvm::dbgs() << (msg) 
#define WARN_ON(condition, msg) if (condition) WARN(msg)

/* TODO: Replace these codes with enum? */
#define ECOMPILE 1

#endif /* __ERROR_HPP__ */
