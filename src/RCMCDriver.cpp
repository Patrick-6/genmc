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
	if (!checkPscAcyclicity())
		return;
	if (userConf->checkWbAcyclicity && !g.isWbAcyclic())
		return;
	if (userConf->printExecGraphs)
		llvm::dbgs() << g << g.modOrder << "\n";
	if (userConf->prettyPrintExecGraphs)
		prettyPrintGraph();
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
	ExecutionGraph initGraph;

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

void RCMCDriver::addToWorklist(StackItemType t, Event e, Event shouldRf,
			       View before,
			       std::vector<EventLabel> &&prefix,
			       std::vector<std::pair<Event, Event> > &&moPlacings,
			       int newMoPos = 0)

{
	auto &lab = currentEG->getEventLabel(e);
	StackItem s(t, lab.getStamp(), e, lab.rf, shouldRf,
		    before, std::move(prefix), std::move(moPlacings), newMoPos);

	workqueue[lab.getStamp()].push_back(std::move(s));
}

StackItem RCMCDriver::getNextItem()
{
	for (auto rit = workqueue.rbegin(); rit != workqueue.rend(); ++rit) {
		if (rit->second.empty())
			continue;

		auto si = rit->second[0];
		rit->second.erase(rit->second.begin());
		return si;
	}
	return StackItem();
}

void RCMCDriver::restrictWorklist(unsigned int stamp)
{
	std::vector<int> idxsToRemove;
	for (auto rit = workqueue.rbegin(); rit != workqueue.rend(); ++rit)
		if (rit->first > stamp && rit->second.empty())
			idxsToRemove.push_back(rit->first); // TODO: break out of loop?

	for (auto &i : idxsToRemove)
		workqueue.erase(i);
}

bool RCMCDriver::worklistContainsPrf(const EventLabel &rLab, const Event &shouldRf,
				     const std::vector<EventLabel> &prefix,
				     const std::vector<std::pair<Event, Event> > &moPlacings)
{
	for (auto it = workqueue.begin(); it != workqueue.end(); ++it) {
		if (it->second.empty() || it->second[0].toRevisit != rLab.pos)
			continue;

		auto &sv = it->second;
		if (std::any_of(sv.rbegin(), sv.rend(), [&](StackItem &si)
				{ return si.type == SRevisit && si.shouldRf == shouldRf &&
					 si.writePrefix == prefix && si.moPlacings == moPlacings; }))
			return true;
	}
	return false;
}

void RCMCDriver::filterWorklistPrf(const EventLabel &rLab, const Event &shouldRf,
				   const std::vector<EventLabel> &prefix,
				   const std::vector<std::pair<Event, Event> > &moPlacings)
{
	auto &g = *currentEG;
	for (auto it = workqueue.begin(); it != workqueue.end(); ++it) {
		if (it->second.empty() || it->second[0].toRevisit != rLab.getPos())
			continue;

		auto &sv = it->second;
		sv.erase(std::remove_if(sv.begin(), sv.end(), [&](StackItem &s)
					{ return s.type == SRevisit && s.shouldRf == shouldRf &&
						 g.equivPrefixes(rLab.getStamp(), s.writePrefix, prefix) &&
						 g.equivPlacings(rLab.getStamp(), s.moPlacings, moPlacings); }),
			 sv.end());
	}
}

void RCMCDriver::restrictGraph(unsigned int stamp)
{
	auto &g = *currentEG;
	auto v = g.getViewFromStamp(stamp);

	/* First, free memory allocated by events that will no longer be in the graph */
	for (auto i = 0u; i < g.threads.size(); i++) {
		for (auto j = v[i] + 1; j < g.maxEvents[i]; j++) {
			auto &lab = g.threads[i].eventList[j];
			if (lab.isMalloc())
				EE->freeRegion(lab.getAddr(), lab.val.IntVal.getLimitedValue());
		}
	}

	/* Then, restrict the graph */
	g.cutToView(v);
	return;
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

	bool validExecution = true;
	do {
		auto p = getNextItem();

		if (p.type == None) {
			for (auto mem : EE->stackMem)
				free(mem); /* No need to clear vector */
			// for (auto mem : EE->heapAllocas)
			// 	free(mem);
			currentEG = oldEG;
			return;
		}
		validExecution = revisitReads(p);
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
	for (auto mem : EE->stackMem)
		free(mem);
	// for (auto mem : EE->heapAllocas)
	// 	free(mem);
	EE->stackMem.clear();
	EE->stackAllocas.clear();
	// EE->heapAllocas.clear();
	// EE->freedMem.clear();

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
Event RCMCDriver::checkForRaces()
{
	auto &g = *currentEG;
	auto &lab = g.getLastThreadLabel(g.currentT);

	/* We only check for races when reads and writes are added */
	BUG_ON(!(lab.isRead() || lab.isWrite()));

	auto racy = Event::getInitializer();
	if (lab.isRead())
		racy = g.findRaceForNewLoad(lab.getPos());
	else
		racy = g.findRaceForNewStore(lab.getPos());

	/* If a race is found and the execution is consistent, return it */
	if (!racy.isInitializer() && isExecutionValid())
		return racy;

	/* Else, if this is a heap allocation, also look for memory errors */
	if (!EE->isHeapAlloca(lab.getAddr()))
		return Event::getInitializer();

	auto before = g.getHbBefore(lab.getPos().prev());
	for (auto i = 0u; i < g.threads.size(); i++)
		for (auto j = 0; j < g.maxEvents[i]; j++) {
			auto &oLab = g.threads[i].eventList[j];
			if (oLab.isFree() && oLab.getAddr() == lab.getAddr())
				return oLab.getPos();
			if (oLab.isMalloc() && oLab.getAddr() <= lab.getAddr() &&
			    lab.getAddr() < oLab.val.PointerVal &&
			    oLab.getIndex() > before[oLab.getThread()])
				return oLab.getPos();
	}
	return Event::getInitializer();
}

std::vector<Event> RCMCDriver::properlyOrderStores(EventAttr attr, llvm::Type *typ, llvm::GenericValue *ptr,
						   std::vector<llvm::GenericValue> expVal,
						   std::vector<Event> &stores)
{
	auto &g = *currentEG;
	auto before = g.getPorfBefore(g.getLastThreadEvent(g.currentT));
	std::vector<Event> valid, conflicting;

	if (attr == ATTR_PLAIN)
		return stores;

	for (auto &s : stores) {
		auto oldVal = EE->loadValueFromWrite(s, typ, ptr);
		if ((attr == ATTR_CAS && !EE->compareValues(typ, oldVal, expVal.back())) ||
		     !g.isStoreReadBySettledRMW(s, ptr, before)) {
			if (g.isStoreReadByExclusiveRead(s, ptr))
				conflicting.push_back(s);
			else
				valid.push_back(s);
		}
	}
	valid.insert(valid.end(), conflicting.begin(), conflicting.end());
	return valid;
}

llvm::GenericValue RCMCDriver::visitThreadSelf(llvm::Type *typ)
{
	auto &g = *currentEG;
	llvm::GenericValue result;

	result.IntVal = llvm::APInt(typ->getIntegerBitWidth(), g.threads[g.currentT].id);
	return result;
}

int RCMCDriver::visitThreadCreate(llvm::Function *calledFun, const llvm::ExecutionContext &SF)
{
	ExecutionGraph &g = *currentEG;
	int tid;

	int c = ++g.threads[g.currentT].globalInstructions;
	if (g.maxEvents[g.currentT] <= c) {
		tid = g.threads.size();
		g.addTCreateToGraph(tid);

		Thread thr(calledFun, tid, g.currentT, SF);
		thr.ECStack.push_back(SF);
		thr.tls = EE->threadLocalVars;
		g.threads.push_back(thr);

		g.addStartToGraph(tid, g.getLastThreadEvent(g.currentT));
	} else {
		tid = g.threads[g.currentT].eventList[c].cid;
		g.getECStack(tid).push_back(SF);
	}
	return tid;
}

llvm::GenericValue RCMCDriver::visitThreadJoin(llvm::Function *F, const llvm::GenericValue &arg)
{
	ExecutionGraph &g = *currentEG;

	int tid = arg.IntVal.getLimitedValue(std::numeric_limits<int>::max());
	if (tid < 0 || int (g.threads.size()) <= tid || tid == g.currentT) {
		std::stringstream ss;
		ss << "ERROR: Invalid TID in pthread_join(): " << tid;
		if (tid == g.currentT)
			ss << " (TID cannot be the same as the calling thread)\n";
		WARN(ss.str());
		abort();
	}

	int c = ++g.threads[g.currentT].globalInstructions;
	if (g.maxEvents[g.currentT] <= c) {
		/* Add a relevant event to the graph */
		g.addTJoinToGraph(tid);
	}

	Event child = g.getLastThreadEvent(tid);
	EventLabel &cLab = g.getEventLabel(child);
	EventLabel &lab = g.threads[g.currentT].eventList[c];
	if (!cLab.isFinish()) {
		/* This thread will remain blocked until the respective child terminates */
		g.threads[g.currentT].isBlocked = true;
	} else {
		lab.rf = child;
		cLab.rfm1.push_back(lab.pos);
		lab.hbView.updateMax(cLab.msgView);
		lab.porfView.updateMax(cLab.porfView);
	}

	/* Return a value indicating that pthread_join() succeeded */
	llvm::GenericValue result;
	result.IntVal = llvm::APInt(F->getReturnType()->getIntegerBitWidth(), 0);
	return result;
}

void RCMCDriver::visitThreadFinish()
{
	ExecutionGraph &g = *currentEG;

	int c = ++g.threads[g.currentT].globalInstructions;
	if (g.maxEvents[g.currentT] <= c && /* Make sure that there is not a failed assume... */
	    !g.threads[g.currentT].isBlocked) {
		g.addFinishToGraph();

		if (g.currentT == 0)
			return;

		Event e = g.getLastThreadEvent(g.currentT);
		EventLabel &lab = g.getEventLabel(e);

		for (auto i = 0u; i < g.threads.size(); i++) {
			Event p = g.getLastThreadEvent(i);
			EventLabel &pLab = g.getEventLabel(p);
			if (pLab.isJoin() && pLab.cid == g.currentT) {
				/* If parent thread is waiting for me, relieve it */
				g.threads[i].isBlocked = false;
				/* Update relevant join event */
				pLab.rf = e;
				lab.rfm1.push_back(pLab.pos);
				pLab.hbView.updateMax(lab.msgView);
				pLab.porfView.updateMax(lab.porfView);
			}
		}
	} /* FIXME: Maybe move view update into thread finish creation? */
	  /* FIXME: Thread return values? */
}

void RCMCDriver::visitFence(llvm::AtomicOrdering ord)
{
	ExecutionGraph &g = *currentEG;

	int c = ++g.threads[g.currentT].globalInstructions;
	if (g.maxEvents[g.currentT] > c)
		return;

	g.addFenceToGraph(ord);
	return;
}

llvm::GenericValue RCMCDriver::visitLoad(EventAttr attr, llvm::AtomicOrdering ord,
					 llvm::GenericValue *addr, llvm::Type *typ,
					 llvm::GenericValue &&cmpVal,
					 llvm::GenericValue &&rmwVal,
					 llvm::AtomicRMWInst::BinOp op)
{
	auto &g = *currentEG;

	/* Is the execution driven by an existing graph? */
	int c = ++g.threads[g.currentT].globalInstructions;
	if (c < g.maxEvents[g.currentT]) {
		auto &lab = g.threads[g.currentT].eventList[c];
		auto val = EE->loadValueFromWrite(lab.rf, typ, addr);
		return val;
	}

	/* Get all stores to this location from which we can read from */
	auto stores = getStoresToLoc(addr);
	auto validStores = properlyOrderStores(attr, typ, addr, {cmpVal}, stores);

	/* ... and add a label with the appropriate store. */
	auto &lab = g.addReadToGraph(attr, ord, addr, typ, validStores[0],
				     std::move(cmpVal), std::move(rmwVal), op);

	/* Check for races */
	if (!checkForRaces().isInitializer()) {
		llvm::dbgs() << "Race detected!\n";
		printResults();
		abort();
	}

	/* Push all the other alternatives choices to the Stack */
	for (auto it = validStores.begin() + 1; it != validStores.end(); ++it)
		addToWorklist(SRead, lab.getPos(), *it, {}, {}, {});
	return EE->loadValueFromWrite(validStores[0], typ, addr);
}

void RCMCDriver::visitStore(EventAttr attr, llvm::AtomicOrdering ord,
			    llvm::GenericValue *addr, llvm::Type *typ,
			    llvm::GenericValue &val)
{
	auto &g = *currentEG;

	int c = ++g.threads[g.currentT].globalInstructions;
	if (g.maxEvents[g.currentT] > c)
		return;

	auto &locMO = g.modOrder[addr];
	auto[begO, endO] = getPossibleMOPlaces(addr, attr == ATTR_FAI || attr == ATTR_CAS);

	/* It is always consistent to add the store at the end of MO */
	auto &sLab = g.addStoreToGraph(attr, ord, addr, typ, val, endO);

	/* Check for races */
	if (!checkForRaces().isInitializer()) {
		llvm::dbgs() << "Race detected!\n";
		printResults();
		abort();
	}

	for (auto it = locMO.begin() + begO; it != locMO.begin() + endO; ++it) {

		/* We cannot place the write just before the write of an RMW */
		if (g.getEventLabel(*it).isRMW())
			continue;

		/* Push the stack item */
		addToWorklist(MOWrite, sLab.getPos(), sLab.rf, {}, {}, {},
			      std::distance(locMO.begin(), it));
	}

	calcRevisits(sLab);
	return;
}

llvm::GenericValue RCMCDriver::visitMalloc(const llvm::GenericValue &argSize)
{
	auto &g = *currentEG;
	llvm::GenericValue allocBegin, allocEnd;

	/* Is the execution driven by an existing graph? */
	int c = ++g.threads[g.currentT].globalInstructions;
	if (c < g.maxEvents[g.currentT]) {
		auto &lab = g.threads[g.currentT].eventList[c];
		allocBegin.PointerVal = lab.getAddr();
		return allocBegin;
	}

	WARN_ON_ONCE(argSize.IntVal.getBitWidth() > 64, "malloc-alignment",
		     "WARNING: malloc()'s alignment larger than 64-bit! Limiting...\n");

	/* Get a fresh address and also store the size of this allocation */
	allocBegin.PointerVal = EE->heapAllocas.empty() ? (char *) EE->globalVars.back() + 1
		                                        : (char *) EE->heapAllocas.back() + 1;
	uint64_t size = argSize.IntVal.getLimitedValue();
	allocEnd.PointerVal = (char *) allocBegin.PointerVal + size;

	/* Track the address allocated*/
	char *ptr = static_cast<char *>(allocBegin.PointerVal);
	for (auto i = 0u; i < size; i++)
		EE->heapAllocas.push_back(ptr + i);

	/* Memory allocations are modeled as read events reading from INIT */
	g.addMallocToGraph(static_cast<llvm::GenericValue *>(allocBegin.PointerVal),
			   allocEnd);

	return allocBegin;
}

void RCMCDriver::visitFree(llvm::GenericValue *ptr)
{
	auto &g = *currentEG;
	llvm::GenericValue val;

	/* Is the execution driven by an existing graph? */
	int c = ++g.threads[g.currentT].globalInstructions;
	if (c < g.maxEvents[g.currentT])
		return;

	/* Attempt to free a NULL pointer */
	if (ptr == NULL)
		return;

	Event m = Event::getInitializer();
	auto before = g.getHbBefore(g.getLastThreadEvent(g.currentT));
	for (auto i = 0u; i < g.threads.size(); i++) {
		for (auto j = 1; j <= before[i]; j++) {
			auto &lab = g.threads[i].eventList[j];
			if (lab.isMalloc() && lab.getAddr() == ptr) {
				m = lab.pos;
				break;
			}
		}
	}

	if (m == Event::getInitializer()) {
		WARN("ERROR: Attempted to free non-malloc'ed memory!\n");
		abort();
	}

	/* FIXME: Check the whole graph */
	/* Check to see whether this memory block has already been freed */
	if (std::find(EE->freedMem.begin(), EE->freedMem.end(), ptr)
	    != EE->freedMem.end()) {
		WARN("ERROR: Double-free error!\n");
		abort();
	}
	EE->freedMem.push_back(ptr);

	val.PointerVal = g.getEventLabel(m).val.PointerVal;

	/* Add a label with the appropriate store */
	g.addFreeToGraph(ptr, val);
	return;
}

void RCMCDriver::visitError(std::string &err)
{
	ExecutionGraph &g = *currentEG;

	/* Is the execution that led to the error consistent? */
	if (!isExecutionValid()) {
		EE->ECStack().clear();
		g.threads[g.currentT].isBlocked = true;
		return;
	}

	auto assertThr = g.currentT;
	auto errorEvent = g.getLastThreadEvent(assertThr);
	auto before = g.getPorfBefore(errorEvent);

	/* Print error trace */
	if (userConf->printErrorTrace) {
		EE->replayExecutionBefore(before);
		g.printTraceBefore(g.getLastThreadEvent(assertThr));
	}

	/* Dump the graph into a file (DOT format) */
	if (userConf->dotFile != "") {
		EE->replayExecutionBefore(before);
		dotPrintToFile(userConf->dotFile, before, errorEvent);
	}

	/* Print results and abort */
	llvm::dbgs() << "Assertion violation: " << err << "\n";
	printResults();
	abort();
}

bool RCMCDriver::calcRevisits(EventLabel &lab)
{
	auto &g = *currentEG;
	auto loads = getRevisitLoads(lab);
	auto pendingRMWs = g.getPendingRMWs(lab);

	if (pendingRMWs.size() > 0)
		loads.erase(std::remove_if(loads.begin(), loads.end(), [&](Event &e)
					{ auto &confLab = g.getEventLabel(pendingRMWs.back());
					  return g.getEventLabel(e).getStamp() > confLab.getStamp(); }),
			    loads.end());

	for (auto &l : loads) {
		auto &rLab = g.getEventLabel(l);
		auto preds = g.getViewFromStamp(rLab.getStamp());

		auto before = g.getPorfBefore(lab.getPos());
		auto[writePrefix, moPlacings] = getPrefixToSaveNotBefore(lab, preds);

		auto writePrefixPos = g.getRfsNotBefore(writePrefix, preds);
		writePrefixPos.insert(writePrefixPos.begin(), lab.getPos());

		if (worklistContainsPrf(rLab, lab.getPos(), writePrefix, moPlacings))
			continue;

		filterWorklistPrf(rLab, lab.getPos(), writePrefix, moPlacings);

		addToWorklist(SRevisit, rLab.getPos(), lab.getPos(), before,
			      std::move(writePrefix), std::move(moPlacings));
	}
	return (!lab.isRMW() || pendingRMWs.empty());
}

bool RCMCDriver::revisitReads(StackItem &p)
{
	auto &g = *currentEG;
	auto &lab = g.getEventLabel(p.toRevisit);

	/* Restrict to the predecessors of the event we are revisiting */
	lab.stamp = p.stamp;
	restrictGraph(lab.getStamp());
	restrictWorklist(lab.getStamp());

	/* Restore events that might have been deleted from the graph */
	switch (p.type) {
	case SRead:
	case SReadLibFunc:
		/* Nothing to restore in this case */
		lab.revisitable = (lab.isRMW() || p.type == SReadLibFunc);
		break;
	case SRevisit:
		/*
		 * Restore the part of the SBRF-prefix of the store that revisits a load,
		 * that is not already present in the graph, the MO edges between that
		 * part and the previous MO, and make that part non-revisitable
		 */
		lab.revisited = true;
		lab.stamp = g.nextStamp(); // TODO: perhaps change this?
		g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix, p.moPlacings);
		break;
	case MOWrite: {
		/* Try a different MO ordering, and also consider reads to revisit */
		auto &locMO = g.modOrder[lab.getAddr()];
		locMO.erase(std::find(locMO.begin(), locMO.end(), lab.getPos()));
		locMO.insert(locMO.begin() + p.moPos, lab.getPos());
		return calcRevisits(lab); }
	case MOWriteLib: {
		auto &locMO = g.modOrder[lab.getAddr()];
		locMO.erase(std::find(locMO.begin(), locMO.end(), lab.getPos()));
		locMO.insert(locMO.begin() + p.moPos, lab.getPos());
		return calcLibRevisits(lab); /* Nothing else to do */ }
	default:
		BUG();
	}

	/*
	 * For the case where an reads-from is changed, change the respective reads-from label
	 * and check whether a part of an RMW should be added
	 */
	g.changeRf(lab, p.shouldRf);

	llvm::GenericValue rfVal = EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.getAddr());
	if (lab.isRead() && lab.isRMW() && (lab.isFAI() || EE->compareValues(lab.valTyp, lab.val, rfVal))) {
		g.currentT = lab.getThread();
		auto newVal = lab.nextVal;
		if (lab.isFAI())
			EE->executeAtomicRMWOperation(newVal, rfVal, lab.nextVal, lab.op);
		auto offsetMO = g.modOrder.getStoreOffset(lab.getAddr(), lab.rf) + 1;
		auto &sLab = g.addStoreToGraph(lab.getAttr(), lab.ord, lab.getAddr(), lab.valTyp, newVal, offsetMO);
		return calcRevisits(sLab);
	}

	if (p.type == SReadLibFunc)
		return calcLibRevisits(lab);

	return true;
}

llvm::GenericValue RCMCDriver::visitLibLoad(EventAttr attr, llvm::AtomicOrdering ord,
					    llvm::GenericValue *addr, llvm::Type *typ,
					    std::string functionName)
{
	auto &g = *currentEG;
	auto lib = Library::getLibByMemberName(getGrantedLibs(), functionName);

	/* Is the execution driven by an existing graph? */
	int c = ++g.threads[g.currentT].globalInstructions;
	if (c < g.maxEvents[g.currentT]) {
		auto &lab = g.threads[g.currentT].eventList[c];
		auto val = EE->loadValueFromWrite(lab.rf, typ, addr);
		return val;
	}

	/* Add the event to the graph so we'll be able to calculate consistent RFs */
	auto &lab = g.addLibReadToGraph(ATTR_PLAIN, ord, addr, typ, Event::getInitializer(), functionName);

	/* Make sure there exists an initializer event for this memory location */
	auto stores(g.modOrder[addr]);
	auto it = std::find_if(stores.begin(), stores.end(),
			       [&g](Event e){ return g.getEventLabel(e).isLibInit(); });

	if (it == stores.end()) {
		WARN("Uninitialized memory location used by library found!\n");
		abort();
	}

	auto preds = g.getViewFromStamp(lab.getStamp());
	auto validStores = g.getLibConsRfsInView(*lib, lab, stores, preds);

	/*
	 * If this is a non-functional library, choose one of the available reads-from
	 * options, push the rest to the stack, and return an appropriate value to
	 * the interpreter
	 */
	if (!lib->hasFunctionalRfs()) {
		BUG_ON(validStores.empty());
		g.changeRf(lab, validStores[0]);
		for (auto it = validStores.begin() + 1; it != validStores.end(); ++it)
			addToWorklist(SRead, lab.getPos(), *it, {}, {}, {});
		return EE->loadValueFromWrite(lab.rf, typ, addr);
	}

	/* Otherwise, first record all the inconsistent options */
	std::copy_if(stores.begin(), stores.end(), std::back_inserter(lab.invalidRfs),
		     [&validStores](Event &e){ return std::find(validStores.begin(),
								validStores.end(), e) == validStores.end(); });

	/* Then, partition the stores based on whether they are read */
	auto invIt = std::partition(validStores.begin(), validStores.end(),
				    [&g](Event &e){ return g.getEventLabel(e).rfm1.empty(); });

	/* Push all options that break RF functionality to the stack */
	for (auto it = invIt; it != validStores.end(); ++it) {
		auto &sLab = g.getEventLabel(*it);
		BUG_ON(sLab.rfm1.size() > 1);
		if (g.getEventLabel(sLab.rfm1.back()).isRevisitable())
			addToWorklist(SReadLibFunc, lab.getPos(), *it, {}, {}, {});
	}

	/* If there is no valid RF, we have to read BOTTOM */
	if (invIt == validStores.begin()) {
		WARN_ONCE("lib-not-always-block", "FIXME: SHOULD NOT ALWAYS BLOCK -- ALSO IN EE\n");
		auto tempRf = Event::getInitializer();
		return EE->loadValueFromWrite(tempRf, typ, addr);
	}

	/*
	 * If BOTTOM is not the only option, push it to inconsistent RFs as well,
	 * choose a valid store to read-from, and push the other alternatives to
	 * the stack
	 */
	WARN_ONCE("lib-check-before-push", "FIXME: CHECK IF IT'S A NON BLOCKING LIB BEFORE PUSHING?\n");
	lab.invalidRfs.push_back(Event::getInitializer());
	g.changeRf(lab, validStores[0]);

	for (auto it = validStores.begin() + 1; it != invIt; ++it)
		addToWorklist(SRead, lab.getPos(), *it, {}, {}, {});

	return EE->loadValueFromWrite(lab.rf, typ, addr);
}

void RCMCDriver::visitLibStore(EventAttr attr, llvm::AtomicOrdering ord, llvm::GenericValue *addr,
			       llvm::Type *typ, llvm::GenericValue &val, std::string functionName,
			       bool isInit)
{
	auto &g = *currentEG;

	int c = ++g.threads[g.currentT].globalInstructions;
	if (g.maxEvents[g.currentT] > c)
		return;

	/* TODO: Make virtual the race-tracking function ?? */

	/*
	 * We need to try all possible MO placements, but after the initialization write,
	 * which is explicitly included in MO, in the case of libraries.
	 */
	auto &locMO = g.modOrder[addr];
	auto begO = 1;
	auto endO = (int) locMO.size();

	/* If there was not any store previously, check if this location was initialized  */
	if (endO == 0 && !isInit) {
		WARN("Uninitialized memory location used by library found!\n");
		abort();
	}

	/* It is always consistent to add a new event at the end of MO */
	auto &sLab = g.addLibStoreToGraph(attr, ord, addr, typ, val, endO, functionName, isInit);

	calcLibRevisits(sLab);

	auto lib = Library::getLibByMemberName(getGrantedLibs(), functionName);
	if (lib && !lib->tracksCoherence())
		return;

	/*
	 * Check for alternative MO placings. Temporarily remove sLab from
	 * MO, find all possible alternatives, and push them to the workqueue
	 */
	locMO.pop_back();
	for (auto i = begO; i < endO; ++i) {
		locMO.insert(locMO.begin() + i, sLab.getPos());

		/* Check consistency for the graph with this MO */
		auto preds = g.getViewFromStamp(sLab.getStamp());
		if (g.isLibConsistentInView(*lib, preds))
			addToWorklist(MOWriteLib, sLab.getPos(), sLab.rf, {}, {}, {}, i);
		locMO.erase(locMO.begin() + i);
	}
	locMO.push_back(sLab.getPos());
	return;
}

bool RCMCDriver::calcLibRevisits(EventLabel &lab)
{
	/* Get the library of the event causing the revisiting */
	auto &g = *currentEG;
	auto lib = Library::getLibByMemberName(getGrantedLibs(), lab.functionName);
	auto valid = true;
	std::vector<Event> loads, stores;

	/*
	 * If this is a read of a functional library causing the revisit,
	 * then this is a functionality violation: we need to find the conflicting
	 * event, and find an alternative reads-from edge for it
	 */
	if (lib->hasFunctionalRfs() && lab.isRead()) {
		auto conf = g.getPendingLibRead(lab);
		loads = {conf};
		stores = g.getEventLabel(conf).invalidRfs;
		valid = false;
	} else {
		/* It is a normal store -- we need to find revisitable loads */
		loads = g.getRevisitable(lab);
		stores = {lab.getPos()};
	}

	/* Next, find which of the 'stores' can be read by 'loads' */
	auto before = g.getPorfBefore(lab.getPos());
	for (auto &l : loads) {
		/* Calculate the view of the resulting graph */
		auto &rLab = g.getEventLabel(l);
		auto preds = g.getViewFromStamp(rLab.getStamp());
		auto v = preds;
		v.updateMax(before);

		/* Check if this the resulting graph is consistent */
		auto rfs = g.getLibConsRfsInView(*lib, rLab, stores, v);

		for (auto &rf : rfs) {
			/* Push consistent options to stack */
			auto writePrefix = g.getPrefixLabelsNotBefore(before, preds);
			auto moPlacings = (lib->tracksCoherence()) ? g.getMOPredsInBefore(writePrefix, preds)
				                                   : std::vector<std::pair<Event, Event> >();
			auto writePrefixPos = g.getRfsNotBefore(writePrefix, preds);
			writePrefixPos.insert(writePrefixPos.begin(), lab.getPos());

			if (worklistContainsPrf(rLab, rf, writePrefix, moPlacings))
				continue;

			filterWorklistPrf(rLab, rf, writePrefix, moPlacings);

			addToWorklist(SRevisit, rLab.getPos(), rf, before,
				      std::move(writePrefix), std::move(moPlacings));

		}
	}
	return valid;
}

void RCMCDriver::prettyPrintGraph()
{
	auto &g = *currentEG;
	for (auto i = 0u; i < g.threads.size(); i++) {
		auto &thr = g.threads[i];
		llvm::dbgs() << "<" << thr.parentId << "," << thr.id
			     << "> " << thr.threadFun->getName() << ": ";
		for (auto j = 0; j < g.maxEvents[i]; j++) {
			auto &lab = thr.eventList[j];
			if (lab.isRead()) {
				if (lab.isRevisitable())
					llvm::dbgs().changeColor(llvm::buffer_ostream::Colors::GREEN);
				auto val = EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.getAddr());
				llvm::dbgs() << lab.type << EE->getGlobalName(lab.getAddr()) << ","
					     << val.IntVal << " ";
				llvm::dbgs().resetColor();
			} else if (lab.isWrite()) {
				llvm::dbgs() << lab.type << EE->getGlobalName(lab.getAddr()) << ","
					     << lab.val.IntVal << " ";
			}
		}
		llvm::dbgs() << "\n";
	}
	llvm::dbgs() << "\n";
}

void RCMCDriver::dotPrintToFile(std::string &filename, View &before, Event e)
{
	ExecutionGraph &g = *currentEG;
	std::ofstream fout(filename);
	std::string dump;
	llvm::raw_string_ostream ss(dump);

	/* Create a directed graph graph */
	ss << "strict digraph {\n";
	/* Specify node shape */
	ss << "\tnode [shape=box]\n";
	/* Left-justify labels for clusters */
	ss << "\tlabeljust=l\n";
	/* Create a node for the initializer event */
	ss << "\t\"" << Event::getInitializer() << "\"[label=INIT,root=true]\n";

	/* Print all nodes with each thread represented by a cluster */
	for (auto i = 0u; i < before.size(); i++) {
		auto &thr = g.threads[i];
		ss << "subgraph cluster_" << thr.id << "{\n";
		ss << "\tlabel=\"" << thr.threadFun->getName().str() << "()\"\n";
		for (auto j = 1; j <= before[i]; j++) {
			std::stringstream buf;
			auto lab = thr.eventList[j];

			Parser::parseInstFromMData(buf, thr.prefixLOC[j], "");
			ss << "\t" << lab.getPos() << " [label=\"" << buf.str() << "\""
			   << (lab.getPos() == e ? ",style=filled,fillcolor=yellow" : "") << "]\n";
		}
		ss << "}\n";
	}

	/* Print relations between events (po U rf) */
	for (auto i = 0u; i < before.size(); i++) {
		auto &thr = g.threads[i];
		for (auto j = 0; j <= before[i]; j++) {
			std::stringstream buf;
			auto lab = thr.eventList[j];

			Parser::parseInstFromMData(buf, thr.prefixLOC[j], "");
			/* Print a po-edge, but skip dummy start events for
			 * all threads except for the first one */
			if (j < before[i] && !(lab.isStart() && i > 0))
				ss << lab.getPos() << " -> " << lab.getPos().next() << "\n";
			if (lab.isRead())
				ss << "\t" << lab.rf << " -> " << lab.getPos() << "[color=green]\n";
			if (thr.id > 0 && lab.isStart())
				ss << "\t" << lab.rf << " -> " << lab.getPos().next() << "[color=blue]\n";
			if (lab.isJoin())
				ss << "\t" << lab.rf << " -> " << lab.getPos() << "[color=blue]\n";
		}
	}

	ss << "}\n";
	fout << ss.str();
	fout.close();
}


/************************************************************
 ** WEAK RA DRIVER
 ***********************************************************/

std::vector<Event> RCMCDriverWeakRA::getStoresToLoc(llvm::GenericValue *addr)
{
	auto &g = *currentEG;
	auto overwritten = g.findOverwrittenBoundary(addr, g.currentT);
	std::vector<Event> stores;

	if (overwritten.empty()) {
		auto &locMO = g.modOrder[addr];
		stores.push_back(Event::getInitializer());
		stores.insert(stores.end(), locMO.begin(), locMO.end());
		return stores;
	}

	auto before = g.getHbRfBefore(overwritten);
	for (auto i = 0u; i < g.threads.size(); i++) {
		Thread &thr = g.threads[i];
		for (auto j = before[i] + 1; j < g.maxEvents[i]; j++) {
			EventLabel &lab = thr.eventList[j];
			if (lab.isWrite() && lab.getAddr() == addr)
				stores.push_back(lab.getPos());
		}
	}
	return stores;
}

std::pair<int, int> RCMCDriverWeakRA::getPossibleMOPlaces(llvm::GenericValue *addr, bool isRMW)
{
	BUG();
}

std::vector<Event> RCMCDriverWeakRA::getRevisitLoads(EventLabel &lab)
{
	BUG();
}

std::pair<std::vector<EventLabel>, std::vector<std::pair<Event, Event> > >
	  RCMCDriverWeakRA::getPrefixToSaveNotBefore(EventLabel &lab, View &before)
{
	BUG();
}

bool RCMCDriverWeakRA::checkPscAcyclicity()
{
	WARN("Unimplemented!\n");
	abort();
}

bool RCMCDriverWeakRA::isExecutionValid()
{
	WARN("Unimplemented!\n");
	abort();
}


/************************************************************
 ** MO DRIVER
 ***********************************************************/

std::vector<Event> RCMCDriverMO::getStoresToLoc(llvm::GenericValue *addr)
{
	std::vector<Event> stores;

	auto &g = *currentEG;
	auto &locMO = g.modOrder[addr];
	auto before = g.getHbBefore(g.getLastThreadEvent(g.currentT));

	auto [begO, endO] = g.splitLocMOBefore(addr, before);

	/*
	 * If there are not stores (hb;rf?)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (hb;rf?)-before stores.
	 */
	if (begO == 0)
		stores.push_back(Event::getInitializer());
	else
		stores.push_back(*(locMO.begin() + begO - 1));
	stores.insert(stores.end(), locMO.begin() + begO, locMO.begin() + endO);
	return stores;
}

std::pair<int, int> RCMCDriverMO::getPossibleMOPlaces(llvm::GenericValue *addr, bool isRMW)
{
	auto &g = *currentEG;
	auto &pLab = g.getLastThreadLabel(g.currentT);

	if (isRMW) {
		auto offset = g.modOrder.getStoreOffset(addr, pLab.rf) + 1;
		return std::make_pair(offset, offset);
	}

	auto before = g.getHbBefore(pLab.getPos());
	return g.splitLocMOBefore(addr, before);
}

std::vector<Event> RCMCDriverMO::getRevisitLoads(EventLabel &sLab)
{
	auto &g = *currentEG;
	auto ls = g.getRevisitable(sLab);
	auto &locMO = g.modOrder[sLab.getAddr()];

	/* If this store is mo-maximal then we are done */
	if (locMO.back() == sLab.getPos())
		return ls;

	/* Otherwise, we have to exclude (mo;rf?;hb?;sb)-after reads */
	auto optRfs = g.getMoOptRfAfter(sLab.getPos());
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ View before = g.getHbPoBefore(e);
				  return std::any_of(optRfs.begin(), optRfs.end(),
					 [&](Event ev)
					 { return ev.index <= before[ev.thread]; });
				}), ls.end());
	return ls;
}

std::pair<std::vector<EventLabel>, std::vector<std::pair<Event, Event> > >
	  RCMCDriverMO::getPrefixToSaveNotBefore(EventLabel &lab, View &before)
{
	auto writePrefix = currentEG->getPrefixLabelsNotBefore(lab.porfView, before);
	auto moPlacings = currentEG->getMOPredsInBefore(writePrefix, before);
	return std::make_pair(std::move(writePrefix), std::move(moPlacings));
}

bool RCMCDriverMO::checkPscAcyclicity()
{
	switch (userConf->checkPscAcyclicity) {
	case CheckPSCType::nocheck:
		return true;
	case CheckPSCType::weak:
	case CheckPSCType::wb:
		WARN_ONCE("check-mo-psc", "WARNING: The full PSC condition is going "
			  "to be checked for the MO-tracking exploration...\n");
	case CheckPSCType::full:
		return currentEG->isPscAcyclicMO();
	default:
		WARN("Unimplemented model!\n");
		BUG();
	}
}

bool RCMCDriverMO::isExecutionValid()
{
	return currentEG->isPscAcyclicMO();
}


/************************************************************
 ** WB DRIVER
 ***********************************************************/

std::vector<Event> RCMCDriverWB::getStoresToLoc(llvm::GenericValue *addr)
{
	auto &g = *currentEG;
	auto[stores, wb] = g.calcWb(addr);
	auto hbBefore = g.getHbBefore(g.getLastThreadEvent(g.currentT));
	auto porfBefore = g.getPorfBefore(g.getLastThreadEvent(g.currentT));

	// Find the stores from which we can read-from
	std::vector<Event> result;
	for (auto i = 0u; i < stores.size(); i++) {
		bool allowed = true;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wb[i * stores.size() + j] &&
			    g.isWriteRfBefore(hbBefore, stores[j]))
				allowed = false;
		}
		if (allowed)
			result.push_back(stores[i]);
	}

	// Also check the initializer event
	bool allowed = true;
	for (auto j = 0u; j < stores.size(); j++)
		if (g.isWriteRfBefore(hbBefore, stores[j]))
			allowed = false;
	if (allowed)
		result.insert(result.begin(), Event::getInitializer());
	return result;
}

std::pair<int, int> RCMCDriverWB::getPossibleMOPlaces(llvm::GenericValue *addr, bool isRMW)
{
	auto locMOSize = (int) currentEG->modOrder[addr].size();
	return std::make_pair(locMOSize, locMOSize);
}

std::vector<Event> RCMCDriverWB::getRevisitLoads(EventLabel &sLab)
{
	auto &g = *currentEG;
	auto ls = g.getRevisitable(sLab);

	if (!sLab.isRMW())
		return ls;

	/*
	 * If sLab is an RMW, we cannot revisit a read r for which
	 * \exists c_a in C_a .
	 *         (c_a, r) \in (hb;[\lW_x];\lRF^?;hb;po)
	 *
	 * since this will create a cycle in WB
	 */
	auto chain = g.getRMWChainDownTo(sLab, Event::getInitializer());
	auto hbAfterStores = g.getStoresHbAfterStores(sLab.getAddr(), chain);

	auto it = std::remove_if(ls.begin(), ls.end(), [&](Event &l)
				 { auto &pLab = g.getPreviousLabel(l);
				   return std::any_of(hbAfterStores.begin(),
						      hbAfterStores.end(),
						      [&](Event &w){ return g.isWriteRfBefore(pLab.hbView, w); }); });
	ls.erase(it, ls.end());
	return ls;
}

std::pair<std::vector<EventLabel>, std::vector<std::pair<Event, Event> > >
	  RCMCDriverWB::getPrefixToSaveNotBefore(EventLabel &lab, View &before)
{
	auto writePrefix = currentEG->getPrefixLabelsNotBefore(lab.porfView, before);
	return std::make_pair(std::move(writePrefix), std::vector<std::pair<Event, Event>>());
}

bool RCMCDriverWB::checkPscAcyclicity()
{
	switch (userConf->checkPscAcyclicity) {
	case CheckPSCType::nocheck:
		return true;
	case CheckPSCType::weak:
		return currentEG->isPscWeakAcyclicWB();
	case CheckPSCType::wb:
		return currentEG->isPscWbAcyclicWB();
	case CheckPSCType::full:
		return currentEG->isPscAcyclicWB();
	default:
		WARN("Unimplemented model!\n");
		BUG();
	}
}

bool RCMCDriverWB::isExecutionValid()
{
	return currentEG->isPscAcyclicWB();
}
