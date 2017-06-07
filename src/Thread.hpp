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

#ifndef __THREAD_HPP__
#define __THREAD_HPP__

#include "Event.hpp"
#include "Interpreter.h"
#include <llvm/IR/Function.h>

/*
 * Thread class - This class represents each program thread
 */
class Thread {
public:
	int id;
	int parentId;
	llvm::Function *threadFun;
	std::vector<EventLabel> eventList;
	std::vector<llvm::ExecutionContext> ECStack;

	Thread();
	Thread(llvm::Function *F);
	Thread(llvm::Function *F, int id);
	friend std::ostream& operator<<(std::ostream &s, const Thread &t);
};

extern bool dryRun;
extern int initNumThreads;

#endif /* __THREAD_HPP__ */
