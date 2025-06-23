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

#ifndef GENMC_BARRIER_RESULT_CHECKER_PASS_HPP
#define GENMC_BARRIER_RESULT_CHECKER_PASS_HPP

#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

class PassModuleInfo;
enum class BarrierRetResult : std::uint8_t;

class BarrierResultAnalysis : public AnalysisInfoMixin<BarrierResultAnalysis> {
public:
	using Result = std::optional<BarrierRetResult>;
	auto run(Module &M, ModuleAnalysisManager &AM) -> Result;

	auto calculate(Module &M) -> Result;

private:
	friend AnalysisInfoMixin<BarrierResultAnalysis>;
	static inline AnalysisKey Key;

	Result resultsUsed{};
};

class BarrierResultCheckerPass : public PassInfoMixin<BarrierResultCheckerPass> {
public:
	BarrierResultCheckerPass(PassModuleInfo &PMI) : PMI(PMI) {}

	auto run(Module &M, ModuleAnalysisManager &MAM) -> PreservedAnalyses;

private:
	PassModuleInfo &PMI;
};

#endif /* GENMC_BARRIER_RETURN_UNUSED_PASS_HPP */
