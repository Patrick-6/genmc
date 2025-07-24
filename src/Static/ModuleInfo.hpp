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

#ifndef GENMC_MODULE_INFO_HPP
#define GENMC_MODULE_INFO_HPP

#include "ADT/VSet.hpp"
#include "Config/Config.hpp"
#include "ExecutionGraph/LoadAnnotation.hpp"
#include "Static/ModuleID.hpp"
#include "Support/NameInfo.hpp"
#include "Support/SExpr.hpp"
#include <llvm/ADT/BitVector.h>
#include <llvm/ADT/IndexedMap.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>

#include "ADT/value_ptr.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace llvm {
class Value;
};

/*
 * Information kept about the module under test by the interpreter.
 */

/** Source-code level (naming) information for variables */
template <typename Key> struct VariableInfo {

	/* Internal types (not exposed to user programs) for which we might
	 * want to collect naming information */
	using ID = Key;
	using InternalKey = std::string;

	std::unordered_map<ID, std::shared_ptr<NameInfo>> globalInfo;
	std::unordered_map<ID, std::shared_ptr<NameInfo>> localInfo;
	std::unordered_map<InternalKey, std::shared_ptr<NameInfo>> internalInfo;

	void clear()
	{
		globalInfo.clear();
		localInfo.clear();
		internalInfo.clear();
	}
};

/** Annotations for loads used by assume()s for SAVer */
template <typename K, typename V> struct AnnotationInfo {
	using Annot = std::pair<AssumeType, value_ptr<SExpr<V>, SExprCloner<V>>>;
	using AnnotUM = std::unordered_map<K, Annot>;

	void clear() { annotMap.clear(); }

	AnnotUM annotMap;
};

enum class BarrierRetResult : std::uint8_t { Unused, Used };

/** A struct to be used from LLVM passes where different kinds of data can be stored.
 * Different from ModuleInfo as it does not require the module to have assigned IDs.
 */
struct PassModuleInfo {

	PassModuleInfo() = default;

	VariableInfo<llvm::Value *> varInfo;
	AnnotationInfo<llvm::Instruction *, llvm::Value *> annotInfo;
	std::optional<ModelType> determinedMM;
	std::optional<BarrierRetResult> barrierResultsUsed;
};

/** Pack together all useful information like VariableInfo and FsInfo for a
 * given module.
 */
struct ModuleInfo {

	ModuleInfo() = delete;
	ModuleInfo(const llvm::Module &mod);

	void clear();

	/* Collects all IDs for the given module.
	 * Should be manually called after the module is modified */
	void collectIDs();

	/* Assumes only statis information have been collected */
	std::unique_ptr<ModuleInfo> clone(const llvm::Module &mod) const;

	ModuleID idInfo;
	VariableInfo<ModuleID::ID> varInfo;
	AnnotationInfo<ModuleID::ID, ModuleID::ID> annotInfo;
	std::optional<ModelType> determinedMM;
	std::optional<BarrierRetResult> barrierResultsUsed;

private:
	const llvm::Module &mod;
};

#endif /* GENMC_MODULE_INFO_HPP */
