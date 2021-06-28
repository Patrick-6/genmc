/*
 * GenMC -- Generic Model Checking.
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
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#include "ModuleInfo.hpp"
#include "SExpr.hpp"

AnnotationInfo::AnnotationInfo() = default;
AnnotationInfo::~AnnotationInfo() = default;
AnnotationInfo::AnnotationInfo(AnnotationInfo &&other) = default;
AnnotationInfo& AnnotationInfo::operator=(AnnotationInfo &&other) = default;

/*
 * If we ever use different contexts, we have to be very careful when
 * cloning annotation/fs information, as these may contain LLVM
 * type information.
 */
std::unique_ptr<ModuleInfo> ModuleInfo::clone(llvm::ValueToValueMapTy &VMap) const
{
	auto info = LLVM_MAKE_UNIQUE<ModuleInfo>();

	/* Copy variable information */
	for (auto &kv : varInfo.globalInfo) {
		BUG_ON(!VMap.count(kv.first));
		info->varInfo.globalInfo[VMap[kv.first]] = kv.second;
	}
	for (auto &kv : varInfo.localInfo) {
		/* We may have collected information about allocas that got deleted ... */
		if (!VMap.count(kv.first))
			continue;
		info->varInfo.localInfo[VMap[kv.first]] = kv.second;
	}
	for (auto &kv : varInfo.internalInfo)
		info->varInfo.internalInfo[kv.first] = kv.second;

	/* Copy annotation information */
	for (auto &kv : annotInfo.annotMap)
		info->annotInfo.annotMap[
		    (llvm::Instruction*) ((llvm::Value *) VMap[(llvm::Value *) kv.first])] = kv.second->clone();

	/* Copy fs information */
	info->fsInfo.inodeTyp = fsInfo.inodeTyp;
	info->fsInfo.fileTyp = fsInfo.fileTyp;

	BUG_ON(!fsInfo.fds.empty());
	info->fsInfo.fds = fsInfo.fds;

	info->fsInfo.blockSize = fsInfo.blockSize;
	info->fsInfo.blockSize = fsInfo.blockSize;

	info->fsInfo.journalData = fsInfo.journalData;
	info->fsInfo.delalloc = fsInfo.delalloc;

	BUG_ON(fsInfo.fdToFile.size() != 0);
	info->fsInfo.fdToFile = fsInfo.fdToFile;

	BUG_ON(fsInfo.dirInode != nullptr);
	info->fsInfo.dirInode = fsInfo.dirInode;

	for (auto &kv : fsInfo.nameToInodeAddr) {
		BUG_ON(kv.second != (char *) 0xdeadbeef);
		info->fsInfo.nameToInodeAddr[kv.first] = kv.second;
	}

	return info;
}
