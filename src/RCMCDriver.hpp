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

#include <ctime>

class RCMCDriver {

private:
	std::string sourceCode;
	Config *userConf;
	std::unique_ptr<llvm::Module> mod;
	llvm::Interpreter *EE;
	ExecutionGraph *currentEG;
	int explored;
	int duplicates;
	std::unordered_set<std::string> uniqueExecs;
	clock_t start;

	void parseLLVMFile(const std::string &fileName);

public:
	RCMCDriver(Config *conf, clock_t start);
	RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod, clock_t start); /* TODO: Check pass by ref */

	llvm::Module *getModule()  { return mod.get(); };
	ExecutionGraph *getGraph() { return currentEG; };
	void run();
	void parseRun();
	void printResults();
	void handleFinishedExecution(ExecutionGraph &g);

	void visitGraph(ExecutionGraph &g);
	void visitStore(ExecutionGraph &g);
	void visitStoreWeakRA(ExecutionGraph &g);
	void visitStoreMO(ExecutionGraph &g);
	bool visitRMWStore(ExecutionGraph &g);
	bool visitRMWStoreWeakRA(ExecutionGraph &g);
	bool visitRMWStoreMO(ExecutionGraph &g);
	Event tryAddRMWStores(ExecutionGraph &g, std::vector<Event> &ls);
	void revisitReads(ExecutionGraph &g, std::vector<std::vector<Event> > &subsets,
			  std::vector<Event> K0, EventLabel &wLab);
};
