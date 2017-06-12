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

#include "ExecutionGraph.hpp"
#include "Parser.hpp"
#include <llvm/IR/Module.h>

class RCMCDriver {

private:
	std::string sourceCode;
	Config *userConf;
	std::unique_ptr<llvm::Module> mod;
	llvm::Interpreter *EE;
	int explored;
	int duplicates;

	void parseLLVMFile(const std::string &fileName);

public:
	RCMCDriver(Config *conf);
	RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod); /* TODO: Check pass by ref */
	void run();
	void parseRun();
	void printResults();

protected:
	void visitGraph(ExecutionGraph &g);
	void revisitReads(ExecutionGraph &g, std::vector<std::vector<Event> > &subsets,
			  std::vector<Event> K0, EventLabel &wLab);
};
