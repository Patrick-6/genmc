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

#ifndef GENMC_CONFIG_HPP
#define GENMC_CONFIG_HPP

#include "ADT/VSet.hpp"
#include "Config/MemoryModel.hpp"
#include "Config/Verbosity.hpp"
#include "Runtime/InterpreterEnumAPI.hpp"

#include <cstdint>
#include <optional>
#include <string>

enum class SchedulePolicy : std::uint8_t { LTR, WF, WFR, Arbitrary };
enum class BoundType : std::uint8_t { context, round, none };
enum class InputType : std::uint8_t { clang, cargo, rust, llvmir };

struct Config {
	/*** General syntax ***/
	std::vector<std::string> cflags;
	std::string inputFile;
	InputType lang;

	/*** Exploration options ***/
	ModelType model{};
	bool estimate{};
	bool isDepTrackingModel{};
	unsigned int threads{};
	std::optional<unsigned int> bound;
	BoundType boundType{};
	bool LAPOR{};
	bool symmetryReduction{};
	bool helper{};
	bool confirmation{};
	bool finalWrite{};
	bool checkLiveness{};
	bool printErrorTrace{};
	std::string dotFile;
	bool instructionCaching{};
	bool disableRaceDetection{};
	bool disableBAM{};
	bool ipr{};
	bool disableStopOnSystemError{};
	bool warnUnfreedMemory{};
	std::optional<std::string> collectLinSpec;
	std::optional<std::string> checkLinSpec;
	unsigned int maxExtSize{};
	bool dotPrintOnlyClientEvents{};
	bool replayCompletedThreads{};

	/*** Transformation options ***/
	std::optional<unsigned> unroll;
	VSet<std::string> noUnrollFuns;
	bool castElimination{};
	bool inlineFunctions{};
	bool loopJumpThreading{};
	bool spinAssume{};
	bool codeCondenser{};
	bool loadAnnot{};
	bool assumePropagation{};
	bool mmDetector{};

	/*** Debugging options ***/
	unsigned int estimationMax{};
	unsigned int estimationMin{};
	unsigned int sdThreshold{};
	bool printExecGraphs{};
	bool printBlockedExecs{};
	SchedulePolicy schedulePolicy{};
	std::string randomScheduleSeed;
	bool printRandomScheduleSeed{};
	std::string outputLlvmBefore;
	std::string outputLlvmAfter;
	bool disableGenmcStdRebuild{};
	std::string linkWith;
	std::string programEntryFun;
	unsigned int warnOnGraphSize{};
	VerbosityLevel vLevel{};
#ifdef ENABLE_GENMC_DEBUG
	bool printStamps{};
	bool colorAccesses{};
	bool validateExecGraphs{};
	bool countDuplicateExecs{};
	bool countMootExecs{};
	bool printEstimationStats{};
	bool boundsHistogram{};
	bool relincheDebug{};
#endif
};

/* Parses CLI options and initializes a Config object */
void parseConfig(int argc, char **argv, Config &conf);

/* Check validity of config options. */
void checkConfigOptions(Config &conf);

/* Returns the language of the input file */
InputType determineLang(std::string inputFile);

#endif /* GENMC_CONFIG_HPP */
