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

#include "Config/Config.hpp"
#include "Runtime/Interpreter.h"
#include "Static/LLVMModule.hpp"
#include "Support/Error.hpp"
#include "Support/ThreadPool.hpp"
#include "Verification/GenMCDriver.hpp"

#include <llvm/Support/FileSystem.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <unistd.h>

namespace fs = llvm::sys::fs;

static auto durationToMill(const std::chrono::high_resolution_clock::duration &duration)
{
	static constexpr long double secToMillFactor = 1e-3L;
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() *
	       secToMillFactor;
}

static auto getElapsedSecs(const std::chrono::high_resolution_clock::time_point &begin)
	-> long double
{
	return durationToMill(std::chrono::high_resolution_clock::now() - begin);
}

static auto getOutFilename(const std::shared_ptr<const Config> & /*conf*/) -> std::string
{
	static char filenameTemplate[] = "/tmp/__genmc.ll.XXXXXX";
	static bool createdFilename = false;

	if (!createdFilename) {
		close(mkstemp(filenameTemplate));
		createdFilename = true;
	}
	return {filenameTemplate};
}

static auto buildCompilationArgs(const std::shared_ptr<const Config> &conf) -> std::string
{
	std::string args;

	args += " -fno-discard-value-names";
	args += " -Xclang";
	args += " -disable-O0-optnone";
	/* We may be linking with Rust which uses dwarf-4 */
	args += !conf->linkWith.empty() ? " -gdwarf-4" : " -g";
	for (const auto &f : conf->cflags)
		args += " " + f;
	args += " -I" SRC_INCLUDE_DIR;
	args += " -I" INCLUDE_DIR;
	args += " -S -emit-llvm";
	args += " -o " + getOutFilename(conf);
	args += " " + conf->inputFile;

	return args;
}

static auto compileInput(const std::shared_ptr<const Config> &conf,
			 const std::unique_ptr<llvm::LLVMContext> &ctx,
			 std::unique_ptr<llvm::Module> &module) -> bool
{
	const auto *path = CLANGPATH;
	auto command = path + buildCompilationArgs(conf);
	if (std::system(command.c_str()) != 0)
		return false;

	module = LLVMModule::parseLLVMModule(getOutFilename(conf), ctx);
	return true;
}

/**
 * Clean + build a given Cargo crate
 */
static auto buildCargoProject(std::string &cargoPath, std::filesystem::path projectPath,
			      bool skipBuild = false) -> bool
{
	if (skipBuild)
		return true;

	/* Save current working directory */
	const std::filesystem::path origWD = std::filesystem::current_path();

	/* Clean + build the cargo project */
	std::filesystem::current_path(projectPath);
	std::string cleanCommand = cargoPath + std::string(" clean");
	if (std::system(cleanCommand.c_str()) != 0)
		WARN("'cargo clean' failed");

	std::string buildCommand =
		std::string("RUSTFLAGS=\"--emit=llvm-bc\" ") + cargoPath + std::string(" build");
	if (std::system(buildCommand.c_str()) != 0) {
		WARN("'cargo build' failed \n");
		return false;
	}

	/* Restore original working directory to allow relative pathnames */
	std::filesystem::current_path(origWD);
	return true;
}

/**
 * Compilation for *.rs input types.
 * Strategy: Build genmc-std as library cargo project separately, then include the produced .rlib
 * file as a prelude (--extern) when building our .rs file with rustc. Link the llvm-ir output files
 * produced by genmc-std into it seperately.
 */
static auto compileRustInput(const std::shared_ptr<const Config> &conf,
			     const std::unique_ptr<llvm::LLVMContext> &ctx,
			     std::unique_ptr<llvm::Module> &module) -> bool
{
#ifndef RUSTC_PATH
	ERROR("Rust not linked, use cmake with '-DRUST_BIN_PATH=...' and rebuild GenMC to use "
	      "Rust.");
#define RUSTC_PATH ""
#define CARGO_PATH ""
#endif
	auto rustcPath = std::string(RUSTC_PATH);
	auto cargoPath = std::string(CARGO_PATH);

	std::filesystem::path pathGenmcStd(std::string(RUST_DIR) + "/genmc-std");
	std::filesystem::path pathDebug = pathGenmcStd / "target" / "debug";
	std::filesystem::path pathRlib = pathDebug / "libgenmc_std.rlib";

	/* Build genmc-std, skip if a build exists that we're allowed to use */
	bool skipBuild = conf->disableGenmcStdRebuild && std::filesystem::exists(pathRlib);
	bool buildSuccessful =
		buildCargoProject(cargoPath, RUST_DIR + std::string("/genmc-std"), skipBuild);
	if (!buildSuccessful)
		return false;

	/* Build args for rustc */
	std::string args;
	args += " --emit=llvm-bc";
	args += " -Copt-level=0";
	for (const auto &f : conf->cflags)
		args += " " + f;
	args += " --extern std=" + pathRlib.string();
	args += " -o rustc_out.bc";
	args += " " + conf->inputFile;

	/* Build the rustc file */
	std::string rustcCommand = rustcPath + args;
	if (std::system(rustcCommand.c_str()) != 0) {
		WARN("'rustc' failed \n");
		return false;
	}

	/* Link genmc-std's llvm-ir output into it (otherwise unused functions like
	 * genmc__rust_alloc are not present in the module for use in later LLVM-Passes) */
	std::string mvCommand = std::string("mv -f rustc_out.bc ") + (pathDebug / "deps").string();
	if (std::system(mvCommand.c_str()) != 0) {
		WARN("Copying the LLVM-IR output failed \n");
		return false;
	}
	module = LLVMModule::parseLinkAllLLVMModules(pathGenmcStd, ctx);

	return true;
}

/**
 * Compilation for Cargo-crate input types.
 */
static auto compileCargoInput(const std::shared_ptr<const Config> &conf,
			      const std::unique_ptr<llvm::LLVMContext> &ctx,
			      std::unique_ptr<llvm::Module> &module) -> bool
{
#ifndef RUSTC_PATH
	ERROR("Rust not linked, use cmake with '-DRUST_BIN_PATH=...' and rebuild GenMC to use "
	      "Rust.");
#define CARGO_PATH ""
#endif
	std::string cargoPath = std::string(CARGO_PATH);
	std::filesystem::path inputFilePath(conf->inputFile);

	/* Save current working directory */
	const std::filesystem::path origWD = std::filesystem::current_path();

	/* Clean + build the cargo project */
	bool buildSuccessful = buildCargoProject(cargoPath, inputFilePath.parent_path());
	if (!buildSuccessful)
		return false;

	/* Restore original working directory to allow relative pathnames */
	std::filesystem::current_path(origWD);

	/* Parse & Link all the LLVM-BC output files */
	module = LLVMModule::parseLinkAllLLVMModules(inputFilePath.parent_path(), ctx);
	return true;
}

static auto compileToModule(const std::shared_ptr<const Config> &conf,
			    const std::unique_ptr<llvm::LLVMContext> &ctx)
	-> std::unique_ptr<llvm::Module>
{
	std::unique_ptr<llvm::Module> module;
	if (conf->lang == InputType::llvmir) {
		module = LLVMModule::parseLLVMModule(conf->inputFile, ctx);
	} else if (conf->lang == InputType::clang && !compileInput(conf, ctx, module)) {
		std::exit(ECOMPILE);
	} else if (conf->lang == InputType::cargo && !compileCargoInput(conf, ctx, module)) {
		std::exit(ECOMPILE);
	} else if (conf->lang == InputType::rust && !compileRustInput(conf, ctx, module)) {
		std::exit(ECOMPILE);
	}
	return std::move(module);
}

static void transformInput(const std::shared_ptr<Config> &conf, llvm::Module &module,
			   ModuleInfo &modInfo)
{
	if (!conf->outputLlvmBefore.empty())
		LLVMModule::printLLVMModule(module, conf->outputLlvmBefore);

	LLVMModule::transformLLVMModule(module, modInfo, conf);
	if (!conf->outputLlvmAfter.empty())
		LLVMModule::printLLVMModule(module, conf->outputLlvmAfter);

	/* Warn if BAM is enabled and barrier results might be used */
	if (!conf->disableBAM && modInfo.barrierResultsUsed.has_value() &&
	    *modInfo.barrierResultsUsed == BarrierRetResult::Used) {
		LOG(VerbosityLevel::Warning)
			<< "Could not determine whether barrier_wait() result is used."
			<< " Use -disable-bam if the order in which threads reach the barrier "
			   "matters.\n";
	}

	/* Perhaps override the MM under which verification will take place */
	if (conf->mmDetector && modInfo.determinedMM.has_value() &&
	    isStrongerThan(*modInfo.determinedMM, conf->model)) {
		conf->model = *modInfo.determinedMM;
		conf->isDepTrackingModel = (conf->model == ModelType::IMM);
		LOG(VerbosityLevel::Tip)
			<< "Automatically adjusting memory model to " << conf->model
			<< ". You can disable this behavior with -disable-mm-detector.\n";
	}
}

static void printEstimationResults(const std::shared_ptr<const Config> &conf,
				   const std::chrono::high_resolution_clock::time_point &begin,
				   const VerificationResult &res)
{
	PRINT(VerbosityLevel::Error) << res.message;
	PRINT(VerbosityLevel::Error)
		<< (res.status == VerificationError::VE_OK ? "*** Estimation complete.\n"
							   : "*** Estimation unsuccessful.\n");

	auto mean = std::llround(res.estimationMean);
	auto sd = std::llround(std::sqrt(res.estimationVariance));
	auto meanTimeSecs = getElapsedSecs(begin) / (res.explored + res.exploredBlocked);
	PRINT(VerbosityLevel::Error)
		<< "Total executions estimate: " << mean << " (+- " << sd << ")\n"
		<< "Time to completion estimate: " << llvm::format("%.2Lf", meanTimeSecs * mean)
		<< "s\n";
	GENMC_DEBUG(if (conf->printEstimationStats) PRINT(VerbosityLevel::Error)
			    << "Estimation moot: " << res.exploredMoot << "\n"
			    << "Estimation blocked: " << res.exploredBlocked << "\n"
			    << "Estimation complete: " << res.explored << "\n";);
}

static void printVerificationResults(const std::shared_ptr<const Config> &conf,
				     const VerificationResult &res)
{
	PRINT(VerbosityLevel::Error) << res.message;
	PRINT(VerbosityLevel::Error)
		<< (res.status == VerificationError::VE_OK
			    ? "*** Verification complete.\nNo errors were detected.\n"
			    : "*** Verification unsuccessful.\n");

	PRINT(VerbosityLevel::Error) << "Number of complete executions explored: " << res.explored;
	GENMC_DEBUG(PRINT(VerbosityLevel::Error)
			    << ((conf->countDuplicateExecs)
					? " (" + std::to_string(res.duplicates) + " duplicates)"
					: ""););
	if (res.boundExceeding) {
		BUG_ON(conf->boundType == BoundType::round);
		PRINT(VerbosityLevel::Error)
			<< " (" + std::to_string(res.boundExceeding) + " exceeded bound)";
	}
	if (res.exploredBlocked != 0U) {
		PRINT(VerbosityLevel::Error)
			<< "\nNumber of blocked executions seen: " << res.exploredBlocked;
	}
	GENMC_DEBUG(
		if (conf->countMootExecs) {
			PRINT(VerbosityLevel::Error) << " (+ " << res.exploredMoot << " mooted)";
		};
		if (conf->boundsHistogram) {
			PRINT(VerbosityLevel::Error) << "\nBounds histogram:";
			auto executions = 0u;
			for (auto i = 0u; i < res.exploredBounds.size(); i++) {
				executions += res.exploredBounds[i];
				PRINT(VerbosityLevel::Error) << " " << executions;
			}
			if (!executions)
				PRINT(VerbosityLevel::Error) << " 0";
		});
	if (conf->checkLinSpec) {
		PRINT(VerbosityLevel::Error)
			<< "\nNumber of checked hints: " << res.relincheResult.hintsChecked;
		GENMC_DEBUG(PRINT(VerbosityLevel::Error) << llvm::format(
				    "\nRelinche time: %.2Lfs",
				    durationToMill(res.relincheResult.analysisTime)););
	}
}

static void calculateHintsAndSaveSpec(const std::shared_ptr<const Config> &conf,
				      const VerificationResult &res)
{
	auto spec = std::move(*res.specification);
	spec.calculateHints();

	PRINT(VerbosityLevel::Error) << "\n*** Specification analysis complete.\n";
	PRINT(VerbosityLevel::Error)
		<< "Number of collected outcomes: " << spec.getNumOutcomes()
		<< " (synchronizations: " << spec.getNumObservations() << ")\n";
	PRINT(VerbosityLevel::Error) << "Number of hints found: " << spec.getNumHints();

	std::error_code err;
	llvm::raw_fd_ostream specFile(*conf->collectLinSpec, err, fs::CD_CreateAlways, fs::FA_Write,
				      fs::OF_None);
	handleFSError(err, "during save specification file");
	serialize(specFile, spec);
}

static auto createExecutionContext(const ExecutionGraph &g) -> std::vector<ThreadInfo>
{
	std::vector<ThreadInfo> tis;
	for (auto i = 1U; i < g.getNumThreads(); i++) { // skip main
		const auto *bLab = g.getFirstThreadLabel(i);
		BUG_ON(!bLab);
		tis.push_back(bLab->getThreadInfo());
	}
	return tis;
}

void run(GenMCDriver *driver, llvm::Interpreter *EE)
{
	do {
		driver->handleExecutionStart();
		if (driver->runFromCache()) {
			driver->handleExecutionEnd();
			continue;
		}
		EE->reset();
		EE->setExecutionContext(createExecutionContext(driver->getExec().getGraph()));
		EE->runAsMain(driver->getConf()->programEntryFun);
		driver->handleExecutionEnd();
	} while (!driver->done());
}

auto estimate(std::shared_ptr<const Config> conf, const std::unique_ptr<llvm::Module> &mod,
	      const std::unique_ptr<ModuleInfo> &modInfo) -> VerificationResult
{
	auto estCtx = std::make_unique<llvm::LLVMContext>();
	auto newmod = LLVMModule::cloneModule(mod, estCtx);
	auto newMI = modInfo->clone(*newmod);
	auto driver = GenMCDriver::create(conf, nullptr,
					  GenMCDriver::EstimationMode{conf->estimationMax});
	std::string buf;
	auto EE = llvm::Interpreter::create(std::move(newmod), std::move(newMI), &*driver,
					    driver->getConf(), driver->getExec().getAllocator(),
					    &buf);
	driver->setEE(&*EE);

	run(&*driver, &*EE);
	return std::move(driver->getResult());
}

auto verify(std::shared_ptr<const Config> conf, std::unique_ptr<llvm::Module> mod,
	    std::unique_ptr<ModuleInfo> modInfo) -> VerificationResult
{
	/* Spawn a single or multiple drivers depending on the configuration */
	if (conf->threads == 1) {
		auto driver = GenMCDriver::create(conf, nullptr, GenMCDriver::VerificationMode{});
		std::string buf;
		auto EE = llvm::Interpreter::create(std::move(mod), std::move(modInfo), &*driver,
						    driver->getConf(),
						    driver->getExec().getAllocator(), &buf);
		driver->setEE(&*EE);
		run(&*driver, &*EE);
		return std::move(driver->getResult());
	}

	std::vector<std::future<VerificationResult>> futures;
	{
		/* Then, fire up the drivers */
		ThreadPool pool(conf, mod, modInfo, run);
		futures = pool.waitForTasks();
	}

	VerificationResult res;
	for (auto &f : futures) {
		res += f.get();
	}
	return res;
}

auto main(int argc, char **argv) -> int
{
	auto begin = std::chrono::high_resolution_clock::now();
	auto conf = std::make_shared<Config>();

	parseConfig(argc, argv, *conf);

	PRINT(VerbosityLevel::Error)
		<< PACKAGE_NAME " v" PACKAGE_VERSION << " (LLVM " LLVM_VERSION ")\n"
		<< "Copyright (C) 2024 MPI-SWS. All rights reserved.\n\n";

	auto ctx = std::make_unique<llvm::LLVMContext>(); // *dtor after module's*
	std::unique_ptr<llvm::Module> module = compileToModule(conf, ctx);

	/* Handle option "-link-with" */
	if (!conf->linkWith.empty()) {
		auto confNew = std::make_shared<Config>(*conf);

		confNew->inputFile = conf->linkWith;
		confNew->lang = determineLang(confNew->inputFile);
		confNew->cflags = {};
		std::unique_ptr<llvm::Module> linkModule = compileToModule(confNew, ctx);

		std::vector<std::unique_ptr<llvm::Module>> modules;
		modules.push_back(std::move(module));
		modules.push_back(std::move(linkModule));
		module = LLVMModule::linkAllModules(std::move(modules));
	}

	PRINT(VerbosityLevel::Error) << "*** Compilation complete.\n";

	/* Perform the necessary transformations */
	auto modInfo = std::make_unique<ModuleInfo>(*module);
	transformInput(conf, *module, *modInfo);
	PRINT(VerbosityLevel::Error) << "*** Transformation complete.\n";

	/* Estimate the state space */
	if (conf->estimate) {
		LOG(VerbosityLevel::Tip) << "Estimating state-space size. For better performance, "
					    "you can use --disable-estimation.\n";
		auto res = estimate(conf, module, modInfo);
		printEstimationResults(conf, begin, res);
		if (res.status != VerificationError::VE_OK)
			return EVERIFY;
	}

	/* Go ahead and try to verify */
	auto res = verify(conf, std::move(module), std::move(modInfo));
	printVerificationResults(conf, res);

	/* Serialize spec if in analysis mode */
	if (conf->collectLinSpec)
		calculateHintsAndSaveSpec(conf, res);

	PRINT(VerbosityLevel::Error)
		<< "\nTotal wall-clock time: " << llvm::format("%.2Lf", getElapsedSecs(begin))
		<< "s\n";

	/* TODO: Check globalContext.destroy() and llvm::shutdown() */
	return res.status == VerificationError::VE_OK ? 0 : EVERIFY;
}
