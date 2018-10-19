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
#include "Error.hpp"
#include "LLVMModule.hpp"
#include "RCMCDriver.hpp"
#include "Interpreter.h"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Format.h>

#include <algorithm>
#include <csignal>
#include <sstream>

/************************************************************
 ** GENERIC RCMC DRIVER
 ***********************************************************/

void abortHandler(int signum)
{
	exit(42);
}

RCMCDriver::RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod,
		       std::vector<Library> &granted, std::vector<Library> &toVerify,
		       clock_t start)
	: userConf(conf), mod(std::move(mod)), grantedLibs(granted), toVerifyLibs(toVerify),
	  explored(0), duplicates(0), start(start)
{
	/*
	 * Make sure we can resolve symbols in the program as well. We use 0
	 * as an argument in order to load the program, not a library. This
	 * is useful as it allows the executions of external functions in the
	 * user code.
	 */
	std::string ErrorStr;
	if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(0, &ErrorStr))
		WARN("Could not resolve symbols in the program: " + ErrorStr);
}

RCMCDriver *RCMCDriver::create(Config *conf, std::unique_ptr<llvm::Module> mod,
			       std::vector<Library> &granted, std::vector<Library> &toVerify,
			       clock_t start)
{
	switch (conf->model) {
	case ModelType::weakra:
		return new RCMCDriverWeakRA(conf, std::move(mod), granted, toVerify, start);
	case ModelType::mo:
		return new RCMCDriverMO(conf, std::move(mod), granted, toVerify, start);
	case ModelType::wb:
		return new RCMCDriverWB(conf, std::move(mod), granted, toVerify, start);
	default:
		WARN("Unsupported model type! Exiting...\n");
		abort();
	}
}

void RCMCDriver::parseRun(Parser &parser)
{
	/* Parse source code from input file and get an LLVM module */
	sourceCode = parser.readFile(userConf->inputFile);
	mod = std::unique_ptr<llvm::Module>(LLVMModule::getLLVMModule(sourceCode));
	run();
}

void RCMCDriver::printResults()
{
	std::stringstream dups;
	dups << " (" << duplicates << " duplicates)";
	llvm::dbgs() << "Number of complete executions explored: " << explored
		     << ((userConf->countDuplicateExecs) ? dups.str() : "") << "\n";
	llvm::dbgs() << "Total wall-clock time: "
		     << llvm::format("%.2f", ((float) clock() - start)/CLOCKS_PER_SEC)
		     << "s\n";
}

void RCMCDriver::handleFinishedExecution(ExecutionGraph &g)
{
	if (!checkPscAcyclicity(g))
		return;
	if (userConf->checkWbAcyclicity && !g.isWbAcyclic())
		return;
	if (userConf->printExecGraphs)
		llvm::dbgs() << g << g.modOrder << "\n";
	if (userConf->prettyPrintExecGraphs)
		g.prettyPrintGraph();
	if (userConf->countDuplicateExecs) {
		std::string exec;
		llvm::raw_string_ostream buf(exec);
		buf << g;
		if (uniqueExecs.find(buf.str()) != uniqueExecs.end())
			++duplicates;
		else
			uniqueExecs.insert(buf.str());
	}
	++explored;
}

void RCMCDriver::run()
{
	std::string buf;

	std::signal(SIGABRT, abortHandler);
	LLVMModule::transformLLVMModule(*mod, userConf);
	if (userConf->transformFile != "")
		LLVMModule::printLLVMModule(*mod, userConf->transformFile);

	/* Create an interpreter for the program's instructions. */
	EE = (llvm::Interpreter *) llvm::Interpreter::create(&*mod, userConf, this, &buf);

	/* Create an initial graph */
	ExecutionGraph initGraph(EE);

	/* Create main thread and start event */
	Thread main = Thread(mod->getFunction("main"), 0);
	main.eventList.push_back(EventLabel(EStart, llvm::Acquire, Event(0, 0), Event::getInitializer()));
	initGraph.threads.push_back(main);
	initGraph.maxEvents[0] = 1;

	/* Explore all graphs and print the results */
	visitGraph(initGraph);
	printResults();
	return;
}

void RCMCDriver::addToWorklist(Event e, StackItem s)
{
	worklist[e].push_back(s);
}

std::pair<Event, StackItem> RCMCDriver::getNextItem()
{
	for (auto rit = workstack.rbegin(); rit != workstack.rend(); ++rit) {
		if (worklist[*rit].size() == 0)
			continue;

		auto si = worklist[*rit].back();
		worklist[*rit].pop_back();
		return std::make_pair(*rit, si);
	}
	return std::make_pair(Event::getInitializer(), StackItem());
}

void RCMCDriver::filterWorklist(const std::vector<int> &preds, const std::vector<int> &storeBefore)
{
	workstack.erase(std::remove_if(workstack.begin(), workstack.end(),
				       [&preds, &storeBefore](Event &e)
				       { return e.index > preds[e.thread] &&
				       (storeBefore.size() == 0 || e.index > storeBefore[e.thread]); }),
			workstack.end());
	// worklist.erase(std::remove_if(worklist.begin(), worklist.end(),
	// 			      [&lab, &storeBefore](decltype(*begin(worklist)) kv)
	// 			      { return kv.first.index > lab.loadPreds[kv.first.thread] &&
	// 				       kv.first.index > storeBefore[kv.first.thread]; }),
	// 	       worklist.end());
}

void RCMCDriver::visitGraph(ExecutionGraph &g)
{
	ExecutionGraph *oldEG = currentEG;
	currentEG = &g;

	/* Reset scheduler */
	g.currentT = 0;
	/* Reset all thread local variables */
	for (auto i = 0u; i < g.threads.size(); i++) {
		g.threads[i].tls = EE->threadLocalVars;
		g.threads[i].globalInstructions = 0;
		g.threads[i].isBlocked = false;
	}

start:
	/* Get main program function and run the program */
	EE->runStaticConstructorsDestructors(false);
	EE->runFunctionAsMain(mod->getFunction("main"), {"prog"}, 0);
	EE->runStaticConstructorsDestructors(true);

	bool validExecution;
	Event toRevisit = Event::getInitializer();
	StackItem p;
	do {
		auto workItem = getNextItem();
		toRevisit = workItem.first;
		p = workItem.second;

		if (toRevisit.isInitializer()) {
			for (auto mem : g.stackAllocas)
				free(mem); /* No need to clear vector */
			for (auto mem : g.heapAllocas)
				free(mem);
			currentEG = oldEG;
			return;
		}

		validExecution = revisitReads(g, toRevisit, p);
		// validExecution = true;
//		llvm::dbgs() << "Popping from stack " << ((p.type == SRead) ? "Read\n" : "Write\n"); llvm::dbgs() << "Graph is " << g << "\n, " << explored << " executions so far\n";

// 		if (p.type == SRead) {
// 			EventLabel &lab = g.getEventLabel(toRevisit);
// 			g.cutToEventView(toRevisit, lab.loadPreds);
// 			EventLabel &lab1 = g.getEventLabel(toRevisit);
// 			filterWorklist(lab1.loadPreds, p.storePorfBefore);

// 			g.changeRf(lab1, p.shouldRf);

// 			std::vector<Event> ls0({toRevisit});
// 			Event added = tryAddRMWStores(g, ls0);
// 			if (!added.isInitializer()) {
// 				bool visitable = visitStore(g);
// 				if (!visitable) {
// //					worklist[toRevisit].pop_back();
// 					validExecution = false;
// 				}
// 			}
// //			llvm::dbgs() << "After restriction: \n" << g << "\n";
// 		} else if (p.type == MOWrite) {
// 			auto &sLab = g.getEventLabel(toRevisit);
// 			g.cutToEventView(sLab.pos, p.preds);
// 			auto &sLab1 = g.getEventLabel(toRevisit);
// 			filterWorklist(p.preds, p.storePorfBefore);
// 			g.modOrder[sLab1.addr] = p.newMO;
// 			pushReadsToRevisit(g, sLab1);
// 		} else if (p.type == GRead) {
// 			auto &lab = g.getEventLabel(toRevisit);
// 			g.cutToEventView(lab.pos, lab.loadPreds);
// 			g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);

// 			if (p.revisitable)
// 				lab.makeRevisitable();
// 			else
// 				lab.makeNotRevisitable();

// 			filterWorklist(g.getEventLabel(toRevisit).loadPreds, p.storePorfBefore);

// 			lab = g.getEventLabel(toRevisit);
// 			auto &confLab = g.getEventLabel(p.oldRf);

// 			g.changeRf(lab, p.shouldRf);

// 			auto lib = Library::getLibByMemberName(getGrantedLibs(), confLab.functionName);
// 			auto candidateRfs = std::vector<Event>(confLab.invalidRfs.begin() + 1,
// 							       confLab.invalidRfs.end());
// 			auto possibleRfs = g.filterLibConstraints(*lib, confLab.pos, candidateRfs);

// 			for (auto &s : possibleRfs) {
// 				auto before = g.getPorfBefore(lab.pos);
// 				auto prefix = g.getPrefixLabelsNotBefore(before, confLab.loadPreds);
// 				auto prefixPos = g.getRfsNotBefore(prefix, confLab.loadPreds);
// 				prefixPos.insert(prefixPos.begin(), lab.pos);

// 				if (confLab.revs.contains(prefixPos))
// 					continue;

// 				auto &rfLab = g.getEventLabel(s);
// 				bool willBeRevisited = true;

// 				confLab.revs.add(prefixPos);
// 				addToWorklist(confLab.pos, StackItem(GRead2, rfLab.pos, before,
// 								     prefix, willBeRevisited));
// 			}

// 			auto &lab2 = g.getEventLabel(p.oldRf);
// 			g.changeRf(lab2, lab2.invalidRfs[0]); // hardcoding bottom seems excellent, right?
// 		} else if (p.type == GRead2) {
// 			auto &lab = g.getEventLabel(toRevisit);
// 			g.cutToEventView(lab.pos, lab.loadPreds);
// 			g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);

// 			if (p.revisitable)
// 				lab.makeRevisitable();
// 			else
// 				lab.makeNotRevisitable();

// 			filterWorklist(g.getEventLabel(toRevisit).loadPreds, p.storePorfBefore);

// 			EventLabel &lab1 = g.getEventLabel(toRevisit);
// 			g.changeRf(lab1, p.shouldRf);
// 		} else {
// 			EventLabel &lab = g.getEventLabel(toRevisit);
// 			g.cutToEventView(lab.pos, lab.loadPreds);
// 			g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);

// 			if (p.revisitable)
// 				lab.makeRevisitable();
// 			else
// 				lab.makeNotRevisitable();

// 			filterWorklist(g.getEventLabel(toRevisit).loadPreds, p.storePorfBefore);
// 			EventLabel &lab1 = g.getEventLabel(toRevisit);

// 			g.changeRf(lab1, p.shouldRf);

// 			std::vector<Event> ls0({toRevisit});
// 			Event added = tryAddRMWStores(g, ls0);
// 			if (!added.isInitializer()) {
// 				bool visitable = visitStore(g);
// 				if (!visitable) {
// //					worklist[toRevisit].pop_back();
// 					validExecution = false;
// 				}
// 			}
// //			llvm::dbgs() << "After restriction: \n" << g << "\n";

// 		}
	} while (!validExecution);

	g.currentT = 0;
	for (unsigned int i = 0; i < g.threads.size(); i++) {
                /* Make sure that all stacks are empty since there may
		 * have been a failed assume on some thread
		 * and a join waiting on that thread.
		 * Joins do not empty ECStacks */
		g.threads[i].ECStack = {};
		g.threads[i].tls = EE->threadLocalVars;
		g.threads[i].isBlocked = false;
		g.threads[i].globalInstructions = 0;
	}
	for (auto mem : g.stackAllocas)
		free(mem);
	for (auto mem : g.heapAllocas)
		free(mem);
	g.stackAllocas.clear();
	g.heapAllocas.clear();
	g.freedMem.clear();

//	worklist[toRevisit].pop_back();
	goto start;
}

/*
 * This function is called to check for races when a new event is added.
 * When a race is detected, we have to actually ensure that the execution is valid,
 * in the sense that it is consistent. Thus, this method relies on overrided
 * virtual methods to check for consistency, depending on the operating mode
 * of the driver.
 */
bool RCMCDriver::checkForRaces(ExecutionGraph &g, EventType typ,
			       llvm::AtomicOrdering ord, llvm::GenericValue *addr)
{
	switch (typ) {
	case ERead:
		return !g.findRaceForNewLoad(ord, addr).isInitializer() && isExecutionValid(g);
	case EWrite:
		return !g.findRaceForNewStore(ord,addr).isInitializer() && isExecutionValid(g);
	default:
		BUG();
	}
}

Event RCMCDriver::tryAddRMWStores(ExecutionGraph &g, std::vector<Event> &ls)
{
	ExecutionGraph *oldEG = currentEG;
	currentEG = &g;
	for (auto &l : ls) {
		EventLabel &lab = g.getEventLabel(l);
		llvm::GenericValue rfVal = EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.addr);
		if (EE->isSuccessfulRMW(lab, rfVal)) {
			g.currentT = lab.pos.thread;
			if (lab.attr == CAS) {
				g.addCASStoreToGraph(lab.ord, lab.addr, lab.nextVal, lab.valTyp);
			} else {
				llvm::GenericValue newVal;
				EE->executeAtomicRMWOperation(newVal, rfVal, lab.nextVal, lab.op);
				g.addRMWStoreToGraph(lab.ord, lab.addr, newVal, lab.valTyp);
			}
			currentEG = oldEG;
			return g.getLastThreadEvent(g.currentT);
		}
	}
	currentEG = oldEG;
	return Event::getInitializer(); /* No to-be-exclusive event found */
}


/************************************************************
 ** WEAK RA DRIVER
 ***********************************************************/

std::vector<Event> RCMCDriverWeakRA::getStoresToLoc(llvm::GenericValue *addr)
{
	return currentEG->getStoresToLocWeakRA(addr);
}

bool RCMCDriverWeakRA::visitStore(ExecutionGraph &g)
{
	BUG();
}

bool RCMCDriverWeakRA::pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
{
	BUG();
}

bool RCMCDriverWeakRA::revisitReads(ExecutionGraph &g, Event &e, StackItem &s)
{
	BUG();
}

bool RCMCDriverWeakRA::checkPscAcyclicity(ExecutionGraph &g)
{
	WARN("Unimplemented!\n");
	abort();
}

bool RCMCDriverWeakRA::isExecutionValid(ExecutionGraph &g)
{
	WARN("Unimplemented!\n");
	abort();
}


/************************************************************
 ** MO DRIVER
 ***********************************************************/

std::vector<Event> RCMCDriverMO::getStoresToLoc(llvm::GenericValue *addr)
{
	return currentEG->getStoresToLocMO(addr);
}

bool RCMCDriverMO::visitStore(ExecutionGraph &g)
{
	auto &sLab = g.getLastThreadLabel(g.currentT);
	auto before = g.getHbBefore(sLab.pos);
	auto[concurrent, previous] = g.splitLocMOBefore(g.modOrder[sLab.addr], before);

	/* Make the driver track this store */
	trackEvent(sLab.pos);

	/* If it is an RMW, appropriately update MO in this location and revisit */
	if (sLab.isRMW()) {
		auto &pLab = g.getPreviousLabel(sLab.pos);
		g.modOrder.addAtLocAfter(sLab.addr, pLab.rf, sLab.pos);
		return pushReadsToRevisit(g, sLab);
	}

	/* Otherwise, we need to try all possible MO placings, and revisit for all of them */
	g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);
	pushReadsToRevisit(g, sLab);

	/* Check for alternative MOs */
	if (concurrent.empty())
		return true;

	/* Push stack items to explore all the alternative MOs */
	for (auto it = concurrent.begin(); it != concurrent.end(); ++it) {
		auto &lab = g.getEventLabel(*it);

		/* We cannot place the write just before the write of an RMW */
		if (lab.isRMW())
			continue;

		/* Construct an alternative MO */
		auto newMO(previous);
		newMO.insert(newMO.end(), concurrent.begin(), it);
		newMO.push_back(sLab.pos);
		newMO.insert(newMO.end(), it, concurrent.end());

		/* Push the stack item */
		addToWorklist(sLab.pos, StackItem(MOWrite, newMO, g.getGraphState()));
	}
	return true;
}

bool RCMCDriverMO::pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
{
	std::vector<Event> loads = g.getRevisitLoadsMO(sLab);

	std::vector<Event> pendingRMWs;
	if (sLab.isRMW()) {
		auto &lab = g.getPreviousLabel(sLab.pos);
		auto val = EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.addr);
		pendingRMWs = EE->getPendingRMWs(lab.pos, lab.rf, lab.addr, val);
	}

	if (pendingRMWs.size() > 0)
		loads.erase(std::remove_if(loads.begin(), loads.end(), [&g, &pendingRMWs](Event &e)
					{ EventLabel &confLab = g.getEventLabel(pendingRMWs.back());
					  return e.index > confLab.loadPreds[e.thread]; }),
			    loads.end());

	for (auto &l : loads) {
		EventLabel &rLab = g.getEventLabel(l);

		auto before = g.getPorfBefore(sLab.pos);
		auto writePrefix = g.getPrefixLabelsNotBefore(before, rLab.loadPreds);

		auto moPlacings = g.getMOPredsInBefore(writePrefix, rLab.loadPreds);
		auto writePrefixPos = g.getRfsNotBefore(writePrefix, rLab.loadPreds);
		writePrefixPos.insert(writePrefixPos.begin(), sLab.pos);

		if (rLab.revs.contains(writePrefixPos, moPlacings))
			continue;

		bool willBeRevisited = false;
		if (sLab.isRMW() && pendingRMWs.size() > 0 && l == pendingRMWs.back())
			willBeRevisited = true;

		rLab.revs.add(writePrefixPos, moPlacings);
		addToWorklist(l, StackItem(SWrite, sLab.pos, before,
					   writePrefix, moPlacings, willBeRevisited));
	}
	return (!sLab.isRMW() || pendingRMWs.empty());
}

bool RCMCDriverMO::revisitReads(ExecutionGraph &g, Event &toRevisit, StackItem &p)
{
	EventLabel &lab = g.getEventLabel(toRevisit);

	switch (p.type) {
	case SRead: {
		g.cutToEventView(toRevisit, lab.loadPreds);
		EventLabel &lab1 = g.getEventLabel(toRevisit);
		filterWorklist(lab1.loadPreds, p.storePorfBefore);

		g.changeRf(lab1, p.shouldRf);

		std::vector<Event> ls0({toRevisit});
		Event added = tryAddRMWStores(g, ls0);
		if (!added.isInitializer())
			return visitStore(g);
//			llvm::dbgs() << "After restriction: \n" << g << "\n";
		return true;
	}
	case MOWrite: {
		g.cutToEventView(lab.pos, p.preds);
		auto &sLab1 = g.getEventLabel(toRevisit);
		filterWorklist(p.preds, p.storePorfBefore);
		g.modOrder[sLab1.addr] = p.newMO;
		pushReadsToRevisit(g, sLab1);
		return true;
	}
	case SWrite: {
		g.cutToEventView(lab.pos, lab.loadPreds);
		g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);

		if (p.revisitable)
			lab.makeRevisitable();
		else
			lab.makeNotRevisitable();

		filterWorklist(g.getEventLabel(toRevisit).loadPreds, p.storePorfBefore);
		EventLabel &lab1 = g.getEventLabel(toRevisit);

		g.changeRf(lab1, p.shouldRf);

		std::vector<Event> ls0({toRevisit});
		Event added = tryAddRMWStores(g, ls0);
		if (!added.isInitializer())
			return visitStore(g);
//			llvm::dbgs() << "After restriction: \n" << g << "\n";
		return true;
	}
	default:
		BUG();
	}
}

bool RCMCDriverMO::checkPscAcyclicity(ExecutionGraph &g)
{
	switch (userConf->checkPscAcyclicity) {
	case CheckPSCType::nocheck:
		return true;
	case CheckPSCType::weak:
	case CheckPSCType::wb:
		WARN_ONCE("check-mo-psc", "WARNING: The full PSC condition is going "
			  "to be checked for the MO-tracking exploration...\n");
	case CheckPSCType::full:
		return g.isPscAcyclicMO();
	default:
		WARN("Unimplemented model!\n");
		BUG();
	}
}

bool RCMCDriverMO::isExecutionValid(ExecutionGraph &g)
{
	return g.isPscAcyclicMO();
}


/************************************************************
 ** WB DRIVER
 ***********************************************************/

std::vector<Event> RCMCDriverWB::getStoresToLoc(llvm::GenericValue *addr)
{
	return currentEG->getStoresToLocWB(addr);
}

bool RCMCDriverWB::visitStore(ExecutionGraph &g)
{
	auto &sLab = g.getLastThreadLabel(g.currentT);

	g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);
	return pushReadsToRevisit(g, sLab);
}

bool RCMCDriverWB::pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
{
	auto loads = g.getRevisitLoadsWB(sLab);

	std::vector<Event> pendingRMWs;
	if (sLab.isRMW()) {
		auto &lab = g.getPreviousLabel(sLab.pos);
		auto val = EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.addr);
		pendingRMWs = EE->getPendingRMWs(lab.pos, lab.rf, lab.addr, val);
	}

	if (pendingRMWs.size() > 0)
		loads.erase(std::remove_if(loads.begin(), loads.end(), [&g, &pendingRMWs](Event &e)
					{ EventLabel &confLab = g.getEventLabel(pendingRMWs.back());
					  return e.index > confLab.loadPreds[e.thread]; }),
			    loads.end());

	for (auto &l : loads) {
		EventLabel &rLab = g.getEventLabel(l);

		auto before = g.getPorfBefore(sLab.pos);
		auto writePrefix = g.getPrefixLabelsNotBefore(before, rLab.loadPreds);

		auto writePrefixPos = g.getRfsNotBefore(writePrefix, rLab.loadPreds);
		writePrefixPos.insert(writePrefixPos.begin(), sLab.pos);

		if (rLab.revs.contains(writePrefixPos))
			continue;

		bool willBeRevisited = false;
		if (sLab.isRMW() && pendingRMWs.size() > 0 && l == pendingRMWs.back())
			willBeRevisited = true;

		rLab.revs.add(writePrefixPos);
		addToWorklist(l, StackItem(SWrite, sLab.pos, before, writePrefix, willBeRevisited));
	}
	return (!sLab.isRMW() || pendingRMWs.empty());
}

bool RCMCDriverWB::revisitReads(ExecutionGraph &g, Event &toRevisit, StackItem &p)
{
	EventLabel &lab = g.getEventLabel(toRevisit);

	switch (p.type) {
	case SRead: {
		g.cutToEventView(toRevisit, lab.loadPreds);
		EventLabel &lab1 = g.getEventLabel(toRevisit);
		filterWorklist(lab1.loadPreds, p.storePorfBefore);

		g.changeRf(lab1, p.shouldRf);

		std::vector<Event> ls0({toRevisit});
		Event added = tryAddRMWStores(g, ls0);
		if (!added.isInitializer())
			return visitStore(g);
//			llvm::dbgs() << "After restriction: \n" << g << "\n";
		return true;
	}
	case SWrite: {
		g.cutToEventView(lab.pos, lab.loadPreds);
		g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);

		if (p.revisitable)
			lab.makeRevisitable();
		else
			lab.makeNotRevisitable();

		filterWorklist(g.getEventLabel(toRevisit).loadPreds, p.storePorfBefore);
		EventLabel &lab1 = g.getEventLabel(toRevisit);

		g.changeRf(lab1, p.shouldRf);

		std::vector<Event> ls0({toRevisit});
		Event added = tryAddRMWStores(g, ls0);
		if (!added.isInitializer())
			return visitStore(g);
//			llvm::dbgs() << "After restriction: \n" << g << "\n";
		return true;
	}
	default:
		BUG();
	}
}

bool RCMCDriverWB::checkPscAcyclicity(ExecutionGraph &g)
{
	switch (userConf->checkPscAcyclicity) {
	case CheckPSCType::nocheck:
		return true;
	case CheckPSCType::weak:
		return g.isPscWeakAcyclicWB();
	case CheckPSCType::wb:
		return g.isPscWbAcyclicWB();
	case CheckPSCType::full:
		return g.isPscAcyclicWB();
	default:
		WARN("Unimplemented model!\n");
		BUG();
	}
}

bool RCMCDriverWB::isExecutionValid(ExecutionGraph &g)
{
	return g.isPscAcyclicWB();
}
