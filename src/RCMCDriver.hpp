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

enum StackItemType { SRead, SRevisit, MOWrite, MOWriteLib, SReadLibFunc, None };

struct StackItem {
	StackItemType type;
	unsigned int stamp;
	Event toRevisit;
	Event oldRf;
	Event shouldRf;
	View storePorfBefore;
	std::vector<EventLabel> writePrefix;
	std::vector<std::pair<Event, Event> > moPlacings;
	int moPos;

	StackItem() : type(None) {};
	StackItem(StackItemType t, unsigned int stamp, Event e,
		  Event oldRf, Event shouldRf, View before,
		  std::vector<EventLabel> &&writePrefix,
		  std::vector<std::pair<Event, Event> > &&moPlacings,
		  int newMoPos)
		: type(t), stamp(stamp), toRevisit(e), oldRf(oldRf), shouldRf(shouldRf),
		  storePorfBefore(before), writePrefix(writePrefix), moPlacings(moPlacings),
		  moPos(newMoPos) {};
	// StackItem(StackItemType t, Event shouldRf)
	// 	: type(t), shouldRf(shouldRf) {};
	// StackItem(StackItemType t, std::vector<Event> newMO)
	// 	: type(t), newMO(newMO) {};
	// StackItem(StackItemType t, int newMOPos)
	// 	: type(t), moPos(newMOPos) {};
	// StackItem(StackItemType t, Event shouldRf, View before,
	// 	  std::vector<EventLabel> &writePrefix)
	// 	: type(t), shouldRf(shouldRf), storePorfBefore(before),
	// 	  writePrefix(writePrefix) {};
	// StackItem(StackItemType t, Event shouldRf, View before,
	// 	  std::vector<EventLabel> &writePrefix,
	// 	  std::vector<std::pair<Event, Event> > moPlacings)
	// 	: type(t), shouldRf(shouldRf), storePorfBefore(before),
	// 	  writePrefix(writePrefix), moPlacings(moPlacings) {};
	// StackItem(StackItemType t, Event shouldRf, View before,
	// 	  std::vector<EventLabel> &writePrefix, Event oldRf)
	// 	: type(t), shouldRf(shouldRf), storePorfBefore(before),
	// 	  writePrefix(writePrefix), oldRf(oldRf) {};
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
	std::map<unsigned int, std::vector<StackItem> > workqueue;
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

//	void trackEvent(Event e) { workstack.push_back(e); };
	void addToWorklist(StackItemType t, Event e, Event shouldRf,
			   View before, std::vector<EventLabel> &&prefix,
			   std::vector<std::pair<Event, Event> > &&moPlacings,
			   int newMoPos);
	StackItem getNextItem();
	void restrictWorklist(unsigned int stamp);
	bool worklistContains(const EventLabel &rLab, const std::vector<EventLabel> &prefix,
			      const std::vector<std::pair<Event, Event> > &moPlacings);
	void filterWorklist(const EventLabel &rLab, const std::vector<EventLabel> &prefix,
			    const std::vector<std::pair<Event, Event> > &moPlacings);

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

	llvm::GenericValue visitLoad(llvm::Type *typ, llvm::GenericValue *addr,
				     EventAttr attr, llvm::AtomicOrdering ord,
				     llvm::GenericValue &&cmpVal = llvm::GenericValue(),
				     llvm::GenericValue &&rmwVal = llvm::GenericValue(),
				     llvm::AtomicRMWInst::BinOp op = llvm::AtomicRMWInst::BinOp::BAD_BINOP);
	void visitStore(llvm::Type *typ, llvm::GenericValue *addr, llvm::GenericValue &val,
			EventAttr attr, llvm::AtomicOrdering ord);
	bool calcRevisits(EventLabel &lab);
	llvm::GenericValue visitLibLoad(llvm::Type *typ, llvm::GenericValue *addr,
					EventAttr attr, llvm::AtomicOrdering ord,
					std::string functionName);
	void visitLibStore(llvm::Type *typ, llvm::GenericValue *addr, llvm::GenericValue &val,
			   EventAttr attr, llvm::AtomicOrdering ord, std::string functionName, bool isInit = false);
	bool calcLibRevisits(EventLabel &lab);
	bool revisitReads(StackItem &s);



	virtual std::vector<Event> getStoresToLoc(llvm::GenericValue *addr) = 0;
	virtual std::pair<int, int> getPossibleMOPlaces(llvm::GenericValue *addr, bool isRMW = false) = 0;
	virtual std::vector<Event> getRevisitLoads(EventLabel &lab) = 0;
	virtual std::pair<std::vector<EventLabel>, std::vector<std::pair<Event, Event> > >
			  getPrefixToSaveNotBefore(EventLabel &lab, View &before) = 0;

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
	std::pair<int, int> getPossibleMOPlaces(llvm::GenericValue *addr, bool isRMW);
	std::vector<Event> getRevisitLoads(EventLabel &lab);
	std::pair<std::vector<EventLabel>, std::vector<std::pair<Event, Event> > >
		  getPrefixToSaveNotBefore(EventLabel &lab, View &before);
	// llvm::GenericValue visitLoad(llvm::Type *typ, llvm::GenericValue *addr,
	// 			     EventAttr attr, llvm::AtomicOrdering ord,
	// 			     llvm::GenericValue &&cmpVal = llvm::GenericValue(),
	// 			     llvm::GenericValue &&rmwVal = llvm::GenericValue(),
	// 			     llvm::AtomicRMWInst::BinOp op = llvm::AtomicRMWInst::BinOp::BAD_BINOP);
	// bool visitStore(llvm::Type *typ, llvm::GenericValue *addr, llvm::GenericValue &val,
	// 		EventAttr attr, llvm::AtomicOrdering ord, bool rmwRevisit);
//	bool visitLibStore(ExecutionGraph &g);
//	bool pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab);
//	bool pushLibReadsToRevisit(ExecutionGraph &g, EventLabel &sLab);
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
	std::pair<int, int> getPossibleMOPlaces(llvm::GenericValue *addr, bool isRMW);
	std::vector<Event> getRevisitLoads(EventLabel &lab);
	std::pair<std::vector<EventLabel>, std::vector<std::pair<Event, Event> > >
		  getPrefixToSaveNotBefore(EventLabel &lab, View &before);
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
	std::pair<int, int> getPossibleMOPlaces(llvm::GenericValue *addr, bool isRMW);
	std::vector<Event> getRevisitLoads(EventLabel &lab);
	std::pair<std::vector<EventLabel>, std::vector<std::pair<Event, Event> > >
		  getPrefixToSaveNotBefore(EventLabel &lab, View &before);
	bool checkPscAcyclicity(ExecutionGraph &g);
	bool isExecutionValid(ExecutionGraph &g);
};
