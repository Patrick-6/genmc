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

#ifndef __INTERPRETER_ENUM_API_HPP__
#define __INTERPRETER_ENUM_API_HPP__

#include <unordered_map>

/* Types of allocations in the interpreter */
enum class AddressSpace { AS_User, AS_Internal };

/* Storage types */
enum class Storage { ST_Static, ST_Automatic, ST_Heap, ST_StorageLast };

/* Modeled functions */
enum class InternalFunctions {
	FN_AssertFail,
	FN_RecAssertFail,
	FN_EndLoop,
	FN_Assume,
	FN_NondetInt,
	FN_Malloc,
	FN_Free,
	FN_ThreadSelf,
	FN_ThreadCreate,
	FN_ThreadJoin,
	FN_ThreadExit,
	FN_MutexInit,
	FN_MutexLock,
	FN_MutexUnlock,
	FN_MutexTrylock,
	FN_BeginFS,
	FN_OpenFS,
	FN_CloseFS,
	FN_CreatFS,
	FN_RenameFS,
	FN_LinkFS,
	FN_UnlinkFS,
	FN_TruncateFS,
	FN_ReadFS,
	FN_PreadFS,
	FN_WriteFS,
	FN_PwriteFS,
	FN_FsyncFS,
	FN_SyncFS,
	FN_LseekFS,
	FN_PersBarrierFS,
	FN_EndFS,
};

extern const std::unordered_map<std::string, InternalFunctions> internalFunNames;

#endif /* __INTERPRETER_ENUM_API_HPP__ */
