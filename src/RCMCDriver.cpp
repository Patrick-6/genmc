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

void RCMCDriver::filterWorklist(View &preds)
{
	workstack.erase(std::remove_if(workstack.begin(), workstack.end(),
				       [&preds](Event &e){ return e.index > preds[e.thread]; }),
			workstack.end());
	// worklist.erase(std::remove_if(worklist.begin(), worklist.end(),
	// 			      [&lab, &storeBefore](decltype(*begin(worklist)) kv)
	// 			      { return kv.first.index > lab.preds[kv.first.thread] &&
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

bool RCMCDriverWeakRA::visitLibStore(ExecutionGraph &g)
{
	BUG();
}

bool RCMCDriverWeakRA::pushLibReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
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
		addToWorklist(sLab.pos, StackItem(MOWrite, newMO));
	}
	return true;
}

bool RCMCDriverMO::visitLibStore(ExecutionGraph &g)
{
	auto &sLab = g.getLastThreadLabel(g.currentT);
	auto stores(g.modOrder[sLab.addr]);

	/* Make the driver track this store */
	trackEvent(sLab.pos);

	/* It is always consistent to add a new event at the end of MO */
	g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);
	pushLibReadsToRevisit(g, sLab);

	/* If there was not any store previously, return */
	if (stores.empty()) {
		if (!sLab.isLibInit()) {
			WARN("Uninitialized memory location used by library found!\n");
			abort();
		}
		return true;
	}

	auto lib = Library::getLibByMemberName(getGrantedLibs(), sLab.functionName);
	if (!lib->tracksCoherence())
		return true;

	/*
	 * Check for alternative MO placings. Note that MO_loc has at
	 * least one event (hence the initial loop condition), and
	 * that "stores" does not contain sLab.pos
	 */
	for (auto it = stores.begin() + 1; it != stores.end(); ++it) {
		std::vector<Event> newMO(stores.begin(), it);
		newMO.push_back(sLab.pos);
		newMO.insert(newMO.end(), it, stores.end());

		/* Check consistency for the graph with this MO */
		std::swap(g.modOrder[sLab.addr], newMO);
		if (g.isLibConsistentInView(*lib, sLab.preds))
			addToWorklist(sLab.pos, StackItem(MOWriteLib, g.modOrder[sLab.addr]));
		std::swap(g.modOrder[sLab.addr], newMO);
	}
	return true;
}

bool RCMCDriverMO::pushLibReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
{
	/* Get all reads in graph */
	auto lib = Library::getLibByMemberName(getGrantedLibs(), sLab.functionName);
	auto loads = g.getRevisitable(sLab);

	/* Find which of them are consistent to revisit */
	std::vector<Event> store = {sLab.pos};
	auto before = g.getPorfBefore(sLab.pos);
	for (auto &l : loads) {
		/* Calculate the view of the resulting graph */
		auto &rLab = g.getEventLabel(l);
		auto v(rLab.preds);
		v.updateMax(before);

		/* Check if this the resulting graph is consistent */
		auto valid = g.getLibConsRfsInView(*lib, rLab, store, v);
		if (valid.empty())
			continue;

		/* Push consistent options to stack */
		auto writePrefix = g.getPrefixLabelsNotBefore(before, rLab.preds);
		auto moPlacings = g.getMOPredsInBefore(writePrefix, rLab.preds);
		auto writePrefixPos = g.getRfsNotBefore(writePrefix, rLab.preds);
		writePrefixPos.insert(writePrefixPos.begin(), sLab.pos);

		if (rLab.revs.contains(writePrefixPos, moPlacings))
			continue;

		rLab.revs.add(writePrefixPos, moPlacings);
		addToWorklist(rLab.pos, StackItem(SWrite, sLab.pos, before,
						  writePrefix, moPlacings));
	}
	return true;
}

bool RCMCDriverMO::pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
{
	auto loads = g.getRevisitLoadsMO(sLab);
	auto pendingRMWs = g.getPendingRMWs(sLab);

	if (pendingRMWs.size() > 0)
		loads.erase(std::remove_if(loads.begin(), loads.end(), [&g, &pendingRMWs](Event &e)
					{ EventLabel &confLab = g.getEventLabel(pendingRMWs.back());
					  return e.index > confLab.preds[e.thread]; }),
			    loads.end());

	for (auto &l : loads) {
		EventLabel &rLab = g.getEventLabel(l);

		auto before = g.getPorfBefore(sLab.pos);
		auto writePrefix = g.getPrefixLabelsNotBefore(before, rLab.preds);

		auto moPlacings = g.getMOPredsInBefore(writePrefix, rLab.preds);
		auto writePrefixPos = g.getRfsNotBefore(writePrefix, rLab.preds);
		writePrefixPos.insert(writePrefixPos.begin(), sLab.pos);

		if (rLab.revs.contains(writePrefixPos, moPlacings))
			continue;

		rLab.revs.add(writePrefixPos, moPlacings);
		addToWorklist(l, StackItem(SWrite, sLab.pos, before, writePrefix, moPlacings));
	}
	return (!sLab.isRMW() || pendingRMWs.empty());
}

bool RCMCDriverMO::revisitReads(ExecutionGraph &g, Event &toRevisit, StackItem &p)
{
	EventLabel &lab = g.getEventLabel(toRevisit);
	View cutView(lab.preds);

	/* Restrict to the predecessors of the event we are revisiting */
	g.cutToEventView(lab.pos, lab.preds);
	cutView.updateMax(p.storePorfBefore);
	filterWorklist(cutView);

	/* Restore events that might have been deleted from the graph */
	switch (p.type) {
	case SRead:
		/* Nothing to restore in this case */
		break;
	case SWrite:
		/*
		 * Restore the part of the SBRF-prefix of the store that revisits a load,
		 * that is not already present in the graph, the MO edges between that
		 * part and the previous MO, and make that part non-revisitable
		 */
		g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);
		break;
	case MOWrite:
		/* Try a different MO ordering, and also consider reads to revisit */
		g.modOrder[lab.addr] = p.newMO;
		pushReadsToRevisit(g, lab);
		return true; /* Nothing else to do */
	case MOWriteLib:
		g.modOrder[lab.addr] = p.newMO;
		pushLibReadsToRevisit(g, lab);
		return true; /* Nothing else to do */
	case SReadLibFunc: {
		g.changeRf(lab, p.shouldRf);
		auto &confLab = g.getEventLabel(g.getPendingLibRead(lab)); /* should be after rf-change! */

		View v(confLab.preds);
		View before = g.getPorfBefore(lab.pos);

		v.updateMax(before);

		auto lib = Library::getLibByMemberName(getGrantedLibs(), lab.functionName);
		auto rfs = g.getLibConsRfsInView(*lib, confLab, confLab.invalidRfs, v); /* Bottom shouldn't affect consistency */
//		BUG_ON(rfs.empty());

		for (auto &rf : rfs) {
			auto writePrefix = g.getPrefixLabelsNotBefore(before, confLab.preds);
			auto moPlacings = g.getMOPredsInBefore(writePrefix, confLab.preds);
			auto writePrefixPos = g.getRfsNotBefore(writePrefix, confLab.preds);
			writePrefixPos.insert(writePrefixPos.begin(), lab.pos);

			if (confLab.revs.contains(writePrefixPos, moPlacings))
				continue;

			confLab.revs.add(writePrefixPos, moPlacings);
			addToWorklist(confLab.pos, StackItem(SReadLibFuncRev, rf, before,
							     writePrefix, moPlacings));
		}
		return false;}
	case SReadLibFuncRev:
		g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);
		g.changeRf(lab, p.shouldRf);
		return true;
	default:
		BUG();
	}

	/*
	 * For the case where an reads-from is changed, change the respective reads-from label
	 * and check whether a part of an RMW should be added
	 */
	g.changeRf(lab, p.shouldRf);
	if (g.tryAddRMWStore(g, lab))
		return visitStore(g);

	return true;
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

bool RCMCDriverWB::visitLibStore(ExecutionGraph &g)
{
	auto &sLab = g.getLastThreadLabel(g.currentT);

	if (g.modOrder[sLab.addr].empty() && !sLab.isLibInit()) {
		WARN("Uninitialized memory location used by library found!\n");
		abort();
	}

	g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);
	return pushLibReadsToRevisit(g, sLab);
}

bool RCMCDriverWB::pushLibReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
{
	/* Get all reads in graph */
	auto lib = Library::getLibByMemberName(getGrantedLibs(), sLab.functionName);
	auto loads = g.getRevisitable(sLab);

	/* Find which of them are consistent to revisit */
	std::vector<Event> store = {sLab.pos};
	auto before = g.getPorfBefore(sLab.pos);
	for (auto &l : loads) {

		/* Calculate the view of the resulting graph */
		auto &rLab = g.getEventLabel(l);
		auto v(rLab.preds);
		v.updateMax(before);

		/* Check if this the resulting graph is consistent */
		auto valid = g.getLibConsRfsInView(*lib, rLab, store, v);
		if (valid.empty())
			continue;

		/* Push consistent options to stack */
		auto writePrefix = g.getPrefixLabelsNotBefore(before, rLab.preds);
		auto writePrefixPos = g.getRfsNotBefore(writePrefix, rLab.preds);
		writePrefixPos.insert(writePrefixPos.begin(), sLab.pos);

		if (rLab.revs.contains(writePrefixPos))
			continue;

		rLab.revs.add(writePrefixPos);
		addToWorklist(rLab.pos, StackItem(SWrite, sLab.pos, before, writePrefix));
	}
	return true;
}

bool RCMCDriverWB::pushReadsToRevisit(ExecutionGraph &g, EventLabel &sLab)
{
	auto loads = g.getRevisitLoadsWB(sLab);
	auto pendingRMWs = g.getPendingRMWs(sLab);

	if (pendingRMWs.size() > 0)
		loads.erase(std::remove_if(loads.begin(), loads.end(), [&g, &pendingRMWs](Event &e)
					{ EventLabel &confLab = g.getEventLabel(pendingRMWs.back());
					  return e.index > confLab.preds[e.thread]; }),
			    loads.end());

	for (auto &l : loads) {
		EventLabel &rLab = g.getEventLabel(l);

		auto before = g.getPorfBefore(sLab.pos);
		auto writePrefix = g.getPrefixLabelsNotBefore(before, rLab.preds);

		auto writePrefixPos = g.getRfsNotBefore(writePrefix, rLab.preds);
		writePrefixPos.insert(writePrefixPos.begin(), sLab.pos);

		if (rLab.revs.contains(writePrefixPos))
			continue;

		rLab.revs.add(writePrefixPos);
		addToWorklist(l, StackItem(SWrite, sLab.pos, before, writePrefix));
	}
	return (!sLab.isRMW() || pendingRMWs.empty());
}

bool RCMCDriverWB::revisitReads(ExecutionGraph &g, Event &toRevisit, StackItem &p)
{
	EventLabel &lab = g.getEventLabel(toRevisit);
	View cutView(lab.preds);

	/* Restrict to the predecessors of the event we are revisiting */
	g.cutToEventView(lab.pos, lab.preds);
	cutView.updateMax(p.storePorfBefore);
	filterWorklist(cutView);

	/* Restore events that might have been deleted from the graph */
	switch (p.type) {
	case SRead:
		/* Nothing to restore in this case */
		break;
	case SWrite:
		/*
		 * Restore the part of the SBRF-prefix of the store that revisits a load,
		 * that is not already present in the graph, and make that part
		 * non-revisitable
		 */
		g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);
		break;
	case SReadLibFunc: {
		g.changeRf(lab, p.shouldRf);
		auto &confLab = g.getEventLabel(g.getPendingLibRead(lab)); /* should be after rf-change! */

		View v(confLab.preds);
		View before = g.getPorfBefore(lab.pos);

		v.updateMax(before);

		auto lib = Library::getLibByMemberName(getGrantedLibs(), lab.functionName);
		auto rfs = g.getLibConsRfsInView(*lib, confLab, confLab.invalidRfs, v); /* Bottom shouldn't affect consistency */
//		BUG_ON(rfs.empty());

		for (auto &rf : rfs) {
			auto writePrefix = g.getPrefixLabelsNotBefore(before, confLab.preds);
			auto writePrefixPos = g.getRfsNotBefore(writePrefix, confLab.preds);
			writePrefixPos.insert(writePrefixPos.begin(), lab.pos);

			if (confLab.revs.contains(writePrefixPos))
				continue;

			confLab.revs.add(writePrefixPos);
			addToWorklist(confLab.pos, StackItem(SReadLibFuncRev, rf, before, writePrefix));
		}
		return false;}
	case SReadLibFuncRev:
		g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);
		g.changeRf(lab, p.shouldRf);
		return true;
	default:
		BUG();
	}

	/* Change the reads-from label, and check whether a part of an RMW should be added */
	g.changeRf(lab, p.shouldRf);
	if (g.tryAddRMWStore(g, lab))
		return visitStore(g);

	return true;
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
