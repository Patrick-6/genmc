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
#include "Driver.hpp"
#include "LLVMModule.hpp"
#include "ExecutionGraph.hpp"
#include "Interpreter.h"
#include <llvm/IR/Verifier.h>

Driver::Driver(Config *conf)
{
	userConf = conf;
}

/* TODO: Need to pass by reference? Maybe const? */
void Driver::parseLLVMFile(const std::string &fileName)
{
	Parser fileParser;
	sourceCode = fileParser.readFile(fileName);
}
#include <llvm/Support/Debug.h>
void Driver::run()
{
	std::string buf;
	llvm::Module *mod;
	llvm::ExecutionEngine *EE;

	/* Parse source code from input file and get an LLVM module */
	parseLLVMFile(userConf->IRFile);
	mod = LLVMModule::getLLVMModule(sourceCode);
	LLVMModule::transformLLVMModule(*mod, userConf);
	if (userConf->transformFile != "")
		LLVMModule::printLLVMModule(*mod, userConf->transformFile);

	/* Create an interpreter for the program's instructions. */
	EE = llvm::Interpreter::create(mod, &buf);

	/* Get main program function and run the program */
	EE->runStaticConstructorsDestructors(false);
	EE->runFunctionAsMain(mod->getFunction("main"), {"prog"}, 0);
	EE->runStaticConstructorsDestructors(true);
}

/* TODO: Fix destructors for Driver and config (basically for every class) */
