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

#include "Config.hpp"

Config::Config(int argc, char **argv)
{
	llvm::cl::ParseCommandLineOptions(argc, argv);
}

/* Command-line argument categories */
static llvm::cl::OptionCategory clTransformation("Transformation Options");
static llvm::cl::OptionCategory clDebugging("Debugging Options");

/* Command-line argument types, default values and descriptions */
static llvm::cl::opt<std::string>
clInputFile(llvm::cl::Positional, llvm::cl::Required, llvm::cl::desc("<input file>"));
static llvm::cl::opt<std::string>
clTransformFile("transform-output", llvm::cl::init(""),	llvm::cl::value_desc("file"),
		llvm::cl::desc("Output the transformed LLVM code to transform file"));
static llvm::cl::opt<int>
clLoopUnroll("unroll", llvm::cl::init(-1), llvm::cl::value_desc("N"),
	     llvm::cl::cat(clTransformation),
	     llvm::cl::desc("Unroll loops N times."));
static llvm::cl::opt<bool>
clDisableSpinAssume("disable-spin-assume", llvm::cl::cat(clTransformation),
		    llvm::cl::desc("Disable spin-assume transformation."));
static llvm::cl::opt<bool>
clValidateGraph("validate-graph", llvm::cl::cat(clDebugging),
		llvm::cl::desc("Validate the execution graph in each step."));




/* TODO: Maybe move string functions to another file? */
std::string stripExtension(std::string s)
{
	return s.erase(s.find_last_of("."), std::string::npos);
}

void Config::getConfigOptions(void)
{
	std::string tmp;
	
	inputFile = clInputFile;
	IRFile = stripExtension(clInputFile) + ".ll";
	tmp = "clang -o " + IRFile + " -S -emit-llvm " + inputFile;
	runCommand = tmp.c_str();
	transformFile = clTransformFile;
	unroll = clLoopUnroll;
	spinAssume = !clDisableSpinAssume;
	validateGraph = clValidateGraph;
}

