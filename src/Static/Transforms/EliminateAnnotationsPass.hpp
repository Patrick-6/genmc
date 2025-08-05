/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#ifndef GENMC_ELIMINATE_ANNOTATIONS_PASS_HPP
#define GENMC_ELIMINATE_ANNOTATIONS_PASS_HPP

#include <llvm/Passes/PassBuilder.h>

using namespace llvm;
class Config;

class EliminateAnnotationsPass : public PassInfoMixin<EliminateAnnotationsPass> {
public:
	EliminateAnnotationsPass(const Config *conf) : conf_(conf) {}
	auto run(Function &F, FunctionAnalysisManager &FAM) -> PreservedAnalyses;

private:
	[[nodiscard]] auto getConf() const -> const Config * { return conf_; }

	const Config *conf_;
};

#endif /* GENMC_ELIMINATE_ANNOTATIONS_PASS_HPP */
