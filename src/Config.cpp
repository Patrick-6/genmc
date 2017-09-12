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

#include "config.h"
#include "Config.hpp"

Config::Config(int argc, char **argv) : argc(argc), argv(argv) {}

/* Command-line argument categories */
static llvm::cl::OptionCategory clGeneral("General Options");
static llvm::cl::OptionCategory clTransformation("Transformation Options");
static llvm::cl::OptionCategory clDebugging("Debugging Options");

/* Command-line argument types, default values and descriptions */
llvm::cl::list<std::string>
clCFLAGS(llvm::cl::Positional, llvm::cl::ZeroOrMore, llvm::cl::desc("-- [CFLAGS]"));
static llvm::cl::opt<std::string>
clInputFile(llvm::cl::Positional, llvm::cl::Required, llvm::cl::desc("<input file>"));

llvm::cl::opt<ModelType>
clModelType(llvm::cl::values(
		    clEnumVal(rc11 , "RC11 model"),
		    clEnumVal(wrc11, "Weak RC11 model"),
		    clEnumVal(wb   , "WB model"), NULL),
	    llvm::cl::cat(clGeneral),
	    llvm::cl::desc("Choose model type:"));
static llvm::cl::opt<std::string>
clTransformFile("transform-output", llvm::cl::init(""),	llvm::cl::value_desc("file"),
		llvm::cl::cat(clGeneral),
		llvm::cl::desc("Output the transformed LLVM code to file"));
static llvm::cl::opt<bool>
clPrintErrorTrace("print-error-trace", llvm::cl::cat(clGeneral),
		  llvm::cl::desc("Print error trace"));
static llvm::cl::opt<bool>
clCheckPscAcyclicity("check-psc-acyclicity", llvm::cl::cat(clGeneral),
		     llvm::cl::desc("Perform consistency checks at the end of each execution"));

static llvm::cl::opt<int>
clLoopUnroll("unroll", llvm::cl::init(-1), llvm::cl::value_desc("N"),
	     llvm::cl::cat(clTransformation),
	     llvm::cl::desc("Unroll loops N times"));
static llvm::cl::opt<bool>
clDisableSpinAssume("disable-spin-assume", llvm::cl::cat(clTransformation),
		    llvm::cl::desc("Disable spin-assume transformation"));


static llvm::cl::opt<bool>
clInputFromBitcodeFile("input-from-bitcode-file", llvm::cl::cat(clDebugging),
		       llvm::cl::desc("Read LLVM bitcode directly from file"));
static llvm::cl::opt<bool>
clValidateExecGraphs("validate-exec-graphs", llvm::cl::cat(clDebugging),
		     llvm::cl::desc("Validate the execution graphs in each step"));
static llvm::cl::opt<bool>
clPrintExecGraphs("print-exec-graphs", llvm::cl::cat(clDebugging),
		  llvm::cl::desc("Print explored execution graphs"));
static llvm::cl::opt<bool>
clCountDuplicateExecs("count-duplicate-execs", llvm::cl::cat(clDebugging),
		      llvm::cl::desc("Count duplicate executions (adds runtime overhead)"));

void Config::getConfigOptions(void)
{
	llvm::ArrayRef<const llvm::cl::OptionCategory *> opts =
		{&clGeneral, &clTransformation, &clDebugging};

	/* Hide unrelated LLVM options and parse user configuration */
	llvm::cl::HideUnrelatedOptions(opts);
	llvm::cl::ParseCommandLineOptions(argc, argv);

	/* Save general options */
	cflags.insert(cflags.end(), clCFLAGS.begin(), clCFLAGS.end());
	inputFile = clInputFile;
	model = clModelType;
	transformFile = clTransformFile;
	printErrorTrace = clPrintErrorTrace;
	checkPscAcyclicity = clCheckPscAcyclicity;

	/* Save transformation options */
	unroll = clLoopUnroll;
	spinAssume = !clDisableSpinAssume;

	/* Save debugging options */
	validateExecGraphs = clValidateExecGraphs;
	printExecGraphs = clPrintExecGraphs;
	countDuplicateExecs = clCountDuplicateExecs;
	inputFromBitcodeFile = clInputFromBitcodeFile;
}
