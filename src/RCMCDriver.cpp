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
#include "RCMCDriver.hpp"
#include "LLVMModule.hpp"
#include "ExecutionGraph.hpp"
#include "Interpreter.h"
#include <llvm/IR/Verifier.h>

RCMCDriver::RCMCDriver(Config *conf) : userConf(conf) {}
RCMCDriver::RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod) : userConf(conf), mod(std::move(mod)) {}

/* TODO: Need to pass by reference? Maybe const? */
void RCMCDriver::parseLLVMFile(const std::string &fileName)
{
	Parser fileParser;
	sourceCode = fileParser.readFile(fileName);
}

void RCMCDriver::parseRun()
{
	/* Parse source code from input file and get an LLVM module */
	parseLLVMFile(userConf->inputFile);
	mod = std::unique_ptr<llvm::Module>(LLVMModule::getLLVMModule(sourceCode));
	run();
}

void RCMCDriver::run()
{
	std::string buf;
	llvm::ExecutionEngine *EE;

	LLVMModule::transformLLVMModule(*mod, userConf);
	if (userConf->transformFile != "")
		LLVMModule::printLLVMModule(*mod, userConf->transformFile);

	/* Create an interpreter for the program's instructions. */
	EE = llvm::Interpreter::create(&*mod, userConf, &buf);

	/* Get main program function and run the program */
	EE->runStaticConstructorsDestructors(false);
	EE->runFunctionAsMain(mod->getFunction("main"), {"prog"}, 0);
	EE->runStaticConstructorsDestructors(true);
}

/* TODO: Fix destructors for Driver and config (basically for every class) */
