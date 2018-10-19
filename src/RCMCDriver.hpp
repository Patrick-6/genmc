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

enum StackItemType { SRead, SWrite, MOWrite, GRead, GRead2, None };

struct StackItem {
	StackItemType type;
	Event shouldRf;
	std::vector<int> storePorfBefore;
	std::vector<EventLabel> writePrefix;
	std::vector<Event> newMO;
	std::vector<std::pair<Event, Event> > moPlacings;
	bool revisitable;
	Event oldRf;

	StackItem() : type(None) {};
	StackItem(StackItemType t, Event shouldRf)
		: type(t), shouldRf(shouldRf) {};
	StackItem(StackItemType t, std::vector<Event> newMO)
		: type(t), newMO(newMO) {};
	StackItem(StackItemType t, Event shouldRf, std::vector<int> before,
		  std::vector<EventLabel> &writePrefix, bool revisitable)
		: type(t), shouldRf(shouldRf), storePorfBefore(before),
		  writePrefix(writePrefix), revisitable(revisitable) {};
	StackItem(StackItemType t, Event shouldRf, std::vector<int> before,
		  std::vector<EventLabel> &writePrefix,
		  std::vector<std::pair<Event, Event> > moPlacings,
		  bool revisitable)
		: type(t), shouldRf(shouldRf), storePorfBefore(before),
		  writePrefix(writePrefix), moPlacings(moPlacings),
		  revisitable(revisitable) {};
	StackItem(StackItemType t, Event shouldRf, std::vector<int> before,
		  std::vector<EventLabel> &writePrefix, bool revisitable, Event oldRf)
		: type(t), shouldRf(shouldRf), storePorfBefore(before),
		  writePrefix(writePrefix), revisitable(revisitable), oldRf(oldRf) {};
};

class RCMCDriver {

protected:
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

	RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod,
		   std::vector<Library> &granted, std::vector<Library> &toVerify,
		   clock_t start); /* TODO: Check pass by ref */
public:

	static RCMCDriver *create(Config *conf, std::unique_ptr<llvm::Module> mod,
				  std::vector<Library> &granted,
				  std::vector<Library> &toVerify, clock_t start);

	virtual ~RCMCDriver() {};

	void trackEvent(Event e) { workstack.push_back(e); };
	void addToWorklist(Event e, StackItem s);
	void filterWorklist(View &preds, const std::vector<int> &storeBefore);
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
	bool checkForRaces(ExecutionGraph &g, EventType typ, llvm::AtomicOrdering ord, llvm::GenericValue *addr);
	Event tryAddRMWStores(ExecutionGraph &g, std::vector<Event> &ls);


	virtual std::vector<Event> getStoresToLoc(llvm::GenericValue *addr) = 0;
	virtual bool visitStore(ExecutionGraph &g) = 0;
	virtual bool pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab) = 0;
	virtual bool revisitReads(ExecutionGraph &g, Event &e, StackItem &s) = 0;
	virtual bool checkPscAcyclicity(ExecutionGraph &g) = 0;
	virtual bool isExecutionValid(ExecutionGraph &g) = 0;
};

/* TODO: Fix destructors for Driver and config (basically for every class) */

class RCMCDriverWeakRA : public RCMCDriver {

public:

	RCMCDriverWeakRA(Config *conf, std::unique_ptr<llvm::Module> mod,
			 std::vector<Library> &granted, std::vector<Library> &toVerify,
			 clock_t start)
		: RCMCDriver(conf, std::move(mod), granted, toVerify, start) {};

	std::vector<Event> getStoresToLoc(llvm::GenericValue *addr);
	bool visitStore(ExecutionGraph &g);
	bool pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab);
	bool revisitReads(ExecutionGraph &g, Event &e, StackItem &s);
	bool checkPscAcyclicity(ExecutionGraph &g);
	bool isExecutionValid(ExecutionGraph &g);
};

class RCMCDriverMO : public RCMCDriver {

public:

	RCMCDriverMO(Config *conf, std::unique_ptr<llvm::Module> mod,
		     std::vector<Library> &granted, std::vector<Library> &toVerify,
		     clock_t start)
		: RCMCDriver(conf, std::move(mod), granted, toVerify, start) {};

	std::vector<Event> getStoresToLoc(llvm::GenericValue *addr);
	bool visitStore(ExecutionGraph &g);
	bool pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab);
	bool revisitReads(ExecutionGraph &g, Event &e, StackItem &s);
	bool checkPscAcyclicity(ExecutionGraph &g);
	bool isExecutionValid(ExecutionGraph &g);
};

class RCMCDriverWB : public RCMCDriver {

public:

	RCMCDriverWB(Config *conf, std::unique_ptr<llvm::Module> mod,
		     std::vector<Library> &granted, std::vector<Library> &toVerify,
		     clock_t start)
		: RCMCDriver(conf, std::move(mod), granted, toVerify, start) {};

	std::vector<Event> getStoresToLoc(llvm::GenericValue *addr);
	bool visitStore(ExecutionGraph &g);
	bool pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab);
	bool revisitReads(ExecutionGraph &g, Event &e, StackItem &s);
	bool checkPscAcyclicity(ExecutionGraph &g);
	bool isExecutionValid(ExecutionGraph &g);
};

class LibDriverMO : public RCMCDriverMO {

public:
	void something();
};

class LibDriverWB : public RCMCDriverWB {

public:
	void something();
};
