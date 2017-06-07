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

#include "Thread.hpp"

bool dryRun = true;
int initNumThreads = 0;

Thread::Thread() {}

Thread::Thread(llvm::Function *F) : threadFun(F), parentId(-1), isBlocked(false)
{
	static int _id = 1;

	id = _id++;
	parentId = -1;
}

Thread::Thread(llvm::Function *F, int id) : id(id), threadFun(F), isBlocked(false)
{
	parentId = -1;
}

std::ostream& operator<<(std::ostream &s, const Thread &t)
{
	return s << "Thread (id: " << t.id << ", parent: " << t.parentId << ", function: "
		 << t.threadFun->getName().str() << ")";
}
