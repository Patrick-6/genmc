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

#include "Event.hpp"
#include "EventLabel.hpp"
#include "ExecutionGraph.hpp"
#include "Library.hpp"
#include "Parser.hpp"
#include <llvm/IR/Module.h>

#include <ctime>
#include <unordered_map>
#include <unordered_set>

enum StackItemType { SRead, SWrite, GRead, GRead2, None };

struct StackItem {
	StackItemType type;
	Event shouldRf;
	std::vector<int> storePorfBefore;
	std::vector<EventLabel> writePrefix;
	bool revisitable;
	Event oldRf;

	StackItem() : type(None) {};
	StackItem(StackItemType t, Event shouldRf)
		: type(t), shouldRf(shouldRf) {};
	StackItem(StackItemType t, Event shouldRf, std::vector<int> before,
		  std::vector<EventLabel> &writePrefix, bool revisitable)
		: type(t), shouldRf(shouldRf), storePorfBefore(before),
		  writePrefix(writePrefix), revisitable(revisitable) {};
	StackItem(StackItemType t, Event shouldRf, std::vector<int> before,
		  std::vector<EventLabel> &writePrefix, bool revisitable, Event oldRf)
		: type(t), shouldRf(shouldRf), storePorfBefore(before),
		  writePrefix(writePrefix), revisitable(revisitable), oldRf(oldRf) {};
};

class RCMCDriver {

private:
	std::string sourceCode;
	Config *userConf;
	std::unique_ptr<llvm::Module> mod;
	std::vector<Library> grantedLibs;
	std::vector<Library> toVerifyLibs;
	llvm::Interpreter *EE;
	ExecutionGraph *currentEG;
	std::unordered_map<Event, std::vector<StackItem>, EventHasher > worklist;
	std::vector<Event> workstack;
	int explored;
	int duplicates;
	std::unordered_set<std::string> uniqueExecs;
	clock_t start;

	void parseLLVMFile(const std::string &fileName);

public:
	RCMCDriver(Config *conf, std::vector<Library> &granted,
		   std::vector<Library> &toVerify, clock_t start);
	RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod,
		   std::vector<Library> &granted, std::vector<Library> &toVerify,
		   clock_t start); /* TODO: Check pass by ref */

	void trackRead(Event e) { workstack.push_back(e); };
	void addToWorklist(Event e, StackItem s);
	void filterWorklist(EventLabel lab, std::vector<int> &storeBefore);
	std::pair<Event, StackItem> getNextItem();
	llvm::Module *getModule()  { return mod.get(); };
	std::vector<Library> &getGrantedLibs()  { return grantedLibs; };
	std::vector<Library> &getToVerifyLibs() { return toVerifyLibs; };
	ExecutionGraph *getGraph() { return currentEG; };
	void run();
	void parseRun(Parser &parser);
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
