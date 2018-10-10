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

void RCMCDriver::filterWorklist(EventLabel lab, std::vector<int> &storeBefore)
{
	workstack.erase(std::remove_if(workstack.begin(), workstack.end(),
				       [&lab, &storeBefore](Event &e)
				       { return e.index > lab.loadPreds[e.thread] &&
				       (storeBefore.size() ==0 || e.index > storeBefore[e.thread]); }),
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

		validExecution = true;
//		llvm::dbgs() << "Popping from stack " << ((p.type == SRead) ? "Read\n" : "Write\n"); llvm::dbgs() << "Graph is " << g << "\n, " << explored << " executions so far\n";

		if (p.type == SRead) {
			g.cutToLoad(toRevisit);
			EventLabel &lab1 = g.getEventLabel(toRevisit);
			filterWorklist(g.getEventLabel(toRevisit), p.storePorfBefore);

			g.changeRf(lab1, p.shouldRf);

			std::vector<Event> ls0({toRevisit});
			Event added = tryAddRMWStores(g, ls0);
			if (!added.isInitializer()) {
				bool visitable = visitRMWStore(g);
				if (!visitable) {
//					worklist[toRevisit].pop_back();
					validExecution = false;
				}
			}
//			llvm::dbgs() << "After restriction: \n" << g << "\n";
		} else if (p.type == GRead) {
			auto &lab = g.getEventLabel(toRevisit);
			g.cutToLoad(lab.pos);
			g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix);

			if (p.revisitable)
				lab.makeRevisitable();
			else
				lab.makeNotRevisitable();

			filterWorklist(g.getEventLabel(toRevisit), p.storePorfBefore);

			lab = g.getEventLabel(toRevisit);
			auto &confLab = g.getEventLabel(p.oldRf);

			g.changeRf(lab, p.shouldRf);

			auto lib = Library::getLibByMemberName(getGrantedLibs(), confLab.functionName);
			auto candidateRfs = std::vector<Event>(confLab.invalidRfs.begin() + 1,
							       confLab.invalidRfs.end());
			auto possibleRfs = g.filterLibConstraints(*lib, confLab.pos, candidateRfs);

			for (auto &s : possibleRfs) {
				auto prefix = g.getPrefixLabelsNotBefore(lab.pos, confLab.loadPreds);
				std::vector<Event> prefixPos = {lab.pos};
				std::for_each(prefix.begin(), prefix.end(),
					      [&prefixPos, &confLab](const EventLabel &eLab)
					      { if (eLab.isRead() &&
						    eLab.pos.index > confLab.loadPreds[eLab.pos.thread])
							      prefixPos.push_back(eLab.rf); });

				if (confLab.revs.contains(prefixPos))
					continue;

				auto &rfLab = g.getEventLabel(s);
				bool willBeRevisited = true;

				confLab.revs.add(prefixPos);
				addToWorklist(confLab.pos, StackItem(GRead2, rfLab.pos, g.getPorfBefore(lab.pos),
								     prefix, willBeRevisited));
			}

			auto &lab2 = g.getEventLabel(p.oldRf);
			g.changeRf(lab2, lab2.invalidRfs[0]); // hardcoding bottom seems excellent, right?
		} else if (p.type == GRead2) {
			auto &lab = g.getEventLabel(toRevisit);
			g.cutToLoad(lab.pos);
			g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix);

			if (p.revisitable)
				lab.makeRevisitable();
			else
				lab.makeNotRevisitable();

			filterWorklist(g.getEventLabel(toRevisit), p.storePorfBefore);

			EventLabel &lab1 = g.getEventLabel(toRevisit);
			g.changeRf(lab1, p.shouldRf);
		} else {
			EventLabel &lab = g.getEventLabel(toRevisit);
			g.cutToLoad(lab.pos);
			g.restoreStorePrefix(lab, p.storePorfBefore, p.writePrefix);

			if (p.revisitable)
				lab.makeRevisitable();
			else
				lab.makeNotRevisitable();

			filterWorklist(g.getEventLabel(toRevisit), p.storePorfBefore);
			EventLabel &lab1 = g.getEventLabel(toRevisit);

			g.changeRf(lab1, p.shouldRf);

			std::vector<Event> ls0({toRevisit});
			Event added = tryAddRMWStores(g, ls0);
			if (!added.isInitializer()) {
				bool visitable = visitRMWStore(g);
				if (!visitable) {
//					worklist[toRevisit].pop_back();
					validExecution = false;
				}
			}
//			llvm::dbgs() << "After restriction: \n" << g << "\n";

		}
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

void RCMCDriver::revisitReads(ExecutionGraph &g, std::vector<std::vector<Event> > &subsets,
			      std::vector<Event> K0, EventLabel &wLab)
{
	for (auto &si : subsets) {
		std::vector<Event> ls(si);
		ls.insert(ls.end(), K0.begin(), K0.end());

		if (ls.empty() || (!K0.empty() &&
				   std::find(si.begin(), si.end(), K0.back()) != si.end()))
			continue;

		std::vector<int> after = g.getPorfAfter(ls);
		if (std::any_of(si.begin(), si.end(), [&after](Event &e)
				{ return e.index >= after[e.thread]; }))
			continue;

		std::vector<Event> ls1(K0);
		ls1.erase(std::remove_if(ls1.begin(), ls1.end(), [&si](Event &e)
			  { return std::find(si.begin(), si.end(), e) != si.end(); }), ls1.end());
		ls1.erase(std::remove_if(ls1.begin(), ls1.end(), [&after](Event &e)
			  { return e.index >= after[e.thread]; }), ls1.end());
		ls1.insert(ls1.end(), si.begin(), si.end());

		std::vector<Event> ls0;
		llvm::Interpreter *interp = EE;
		if (std::any_of(si.begin(), si.end(), [interp, &g, &wLab](Event &e)
				{ EventLabel &lab = g.getEventLabel(e);
				  return interp->isSuccessfulRMW(lab, wLab.val); }))
			ls0.insert(ls0.end(), si.begin(), si.end());
		else
			ls0.insert(ls0.end(), ls1.begin(), ls1.end());

		ExecutionGraph eg(g.EE);

		after = g.getPorfAfter(ls1);
//		eg.cutToCopyAfter(g, after);
//		eg.modifyRfs(ls1, wLab.pos);
		std::vector<int> before = eg.getPorfBefore(si);
//		eg.revisit.removePorfBefore(before);

		Event added = tryAddRMWStores(eg, ls0);
		if (!added.isInitializer()) {
			ExecutionGraph *oldEG = currentEG;
			currentEG = &eg;
			bool visitable = visitRMWStore(eg);
			currentEG = oldEG;
			if (visitable)
				visitGraph(eg);
		} else {
			visitGraph(eg);
		}
	}
	return;
}

std::vector<Event> RCMCDriverWeakRA::getStoresToLoc(llvm::GenericValue *addr)
{
	return currentEG->getStoresToLocWeakRA(addr);
}

void RCMCDriverWeakRA::visitStore(ExecutionGraph &g)
{
	Event s = g.getLastThreadEvent(g.currentT);
	EventLabel &sLab = g.getEventLabel(s);

	g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);
	std::vector<Event> ls = g.getRevisitLoads(s);
	std::vector<std::vector<Event > > rSets =
		EE->calcRevisitSets(ls, {}, sLab);
	revisitReads(g, rSets, {}, sLab);
}

bool RCMCDriverWeakRA::visitRMWStore(ExecutionGraph &g)
{
	Event s = g.getLastThreadEvent(g.currentT);
	EventLabel &sLab = g.getEventLabel(s);
	EventLabel &lab = g.getPreviousLabel(s); /* Need to refetch! */

	g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);
	std::vector<Event> ls = g.getRevisitLoads(s);

	llvm::GenericValue val =
		EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.addr);
	std::vector<Event> pendingRMWs =
		EE->getPendingRMWs(lab.pos, lab.rf, lab.addr, val);
	std::vector<std::vector<Event> > rSets =
		EE->calcRevisitSets(ls, pendingRMWs, sLab);

	revisitReads(g, rSets, pendingRMWs, sLab);

	return pendingRMWs.empty();
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

std::vector<Event> RCMCDriverMO::getStoresToLoc(llvm::GenericValue *addr)
{
	return currentEG->getStoresToLocMO(addr);
}

void RCMCDriverMO::visitStore(ExecutionGraph &g)
{
	WARN("Unimplemented!\n");
	abort();
}

bool RCMCDriverMO::visitRMWStore(ExecutionGraph &g)
{
	WARN("Unimplemented!\n");
	abort();
}

bool RCMCDriverMO::checkPscAcyclicity(ExecutionGraph &g)
{
	BUG();
}

bool RCMCDriverMO::isExecutionValid(ExecutionGraph &g)
{
	BUG();
}

std::vector<Event> RCMCDriverWB::getStoresToLoc(llvm::GenericValue *addr)
{
	return currentEG->getStoresToLocWB(addr);
}

void RCMCDriverWB::visitStore(ExecutionGraph &g)
{
	Event store = g.getLastThreadEvent(g.currentT);
	EventLabel &sLab = g.getEventLabel(store);
	std::vector<Event> loads = g.getRevisitLoads(store);

	g.modOrder.addAtLocEnd(sLab.addr, store);
	for (auto &l : loads) {
		EventLabel &rLab = g.getEventLabel(l);

		auto writePrefix = g.getPrefixLabelsNotBefore(store, rLab.loadPreds);
		std::vector<Event> writePrefixPos = {store};

		std::for_each(writePrefix.begin(), writePrefix.end(),
			      [&writePrefixPos, &rLab](const EventLabel &eLab)
			      { if (eLab.isRead() && eLab.pos.index > rLab.loadPreds[eLab.pos.thread])
					      writePrefixPos.push_back(eLab.rf); });

		if (rLab.revs.contains(writePrefixPos))
			continue;

		rLab.revs.add(writePrefixPos);
		addToWorklist(l, StackItem(SWrite, store, g.getPorfBefore(store), writePrefix, false));
	}
}

bool RCMCDriverWB::visitRMWStore(ExecutionGraph &g)
{
	Event s = g.getLastThreadEvent(g.currentT);
	EventLabel &sLab = g.getEventLabel(s);
	EventLabel &lab = g.getPreviousLabel(s); /* Need to refetch! */

	g.modOrder.addAtLocEnd(lab.addr, s);
	llvm::GenericValue val =
		EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.addr);
	std::vector<Event> pendingRMWs =
		EE->getPendingRMWs(lab.pos, lab.rf, lab.addr, val);

	std::vector<Event> ls = g.getRevisitLoadsRMW(s);
	if (pendingRMWs.size() > 0)
		ls.erase(std::remove_if(ls.begin(), ls.end(), [&g, &pendingRMWs](Event &e)
					{ EventLabel &confLab = g.getEventLabel(pendingRMWs.back());
					  return e.index > confLab.loadPreds[e.thread]; }),
			 ls.end());

	for (auto &l : ls) {
		EventLabel &rLab = g.getEventLabel(l);

		auto writePrefix = g.getPrefixLabelsNotBefore(s, rLab.loadPreds);
		std::vector<Event> writePrefixPos = {s};
		std::for_each(writePrefix.begin(), writePrefix.end(),
			      [&writePrefixPos, &rLab](const EventLabel &eLab)
			      { if (eLab.isRead() && eLab.pos.index > rLab.loadPreds[eLab.pos.thread])
					      writePrefixPos.push_back(eLab.rf); });

		if (rLab.revs.contains(writePrefixPos))
			continue;

		bool willBeRevisited = false;
		if (pendingRMWs.size() > 0 && l == pendingRMWs.back())
			willBeRevisited = true;

		rLab.revs.add(writePrefixPos);
		addToWorklist(l, StackItem(SWrite, s, g.getPorfBefore(s), writePrefix, willBeRevisited));
	}


	return pendingRMWs.empty();
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
