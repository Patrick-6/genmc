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

RCMCDriver::RCMCDriver(Config *conf, clock_t start)
	: userConf(conf), explored(0), duplicates(0), start(start)
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
RCMCDriver::RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod, clock_t start)
	: userConf(conf), mod(std::move(mod)), explored(0), duplicates(0), start(start)
{
	std::string ErrorStr;
	if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(0, &ErrorStr))
		WARN("Could not resolve symbols in the program: " + ErrorStr);
}

/* TODO: Need to pass by reference? Maybe const? */
void RCMCDriver::parseLLVMFile(const std::string &fileName)
{
	Parser fileParser;
	sourceCode = fileParser.readFile(fileName);
}

void RCMCDriver::parseRun()
{
	/* Parse source code from input file and get an LLVM module */
	parseLLVMFile(userConf->inputFile);
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
	if (userConf->checkPscAcyclicity && !g.isPscAcyclic())
		return;
	if (userConf->checkWbAcyclicity && !g.isWbAcyclic())
		return;
	if (userConf->printExecGraphs)
		llvm::dbgs() << g << g.revisit << g.modOrder << "\n";
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
	do {
		if (g.workqueue.empty()) {
			for (auto mem : g.stackAllocas)
				free(mem); /* No need to clear vector */
			for (auto mem : g.heapAllocas)
				free(mem);
			currentEG = oldEG;
			return;
		}

		validExecution = true;
		StackItem &p = g.workqueue.back();
		// llvm::dbgs() << "Popping from stack " << ((p.type == RevR) ? "Read\n" : "Write\n"); llvm::dbgs() << "Graph is " << g << "\n, " << explored << " executions so far\n";

		if (p.type == RevR) {
			g.cutBefore(p.preds, p.revisit);
			EventLabel &lab1 = g.getEventLabel(p.e);
			Event oldRf = lab1.rf;
			lab1.rf = p.shouldRf;
			if (!p.shouldRf.isInitializer()) {
				EventLabel &lab2 = g.getEventLabel(p.shouldRf);
				lab2.rfm1.push_front(p.e);
			}
			if (!oldRf.isInitializer()) {
				EventLabel &lab3 = g.getEventLabel(oldRf);
				lab3.rfm1.remove(p.e);
			}

			lab1.hbView = g.getEventHbView(lab1.pos.prev());
			lab1.hbView[lab1.pos.thread] = lab1.pos.index;
			if (lab1.isAtLeastAcquire()) {
				View mV = g.getEventMsgView(lab1.rf);
				lab1.hbView.updateMax(mV);
			}

			std::vector<int> before = g.getPorfBefore(p.e);
			g.revisit.removePorfBefore(before);

			std::vector<Event> ls0({p.e});
			Event added = tryAddRMWStores(g, ls0);
			if (!added.isInitializer()) {
				bool visitable = visitRMWStore(g);
				if (!visitable) {
					g.workqueue.pop_back();
					validExecution = false;
				}
			}
			// llvm::dbgs() << "After restriction: \n" << g << "\n";
		} else {
			EventLabel &sLab = g.getEventLabel(p.e);
			g.cutBefore(p.preds, p.revisit);
			g.modOrder.setLoc(sLab.addr, p.locMO);

			std::vector<Event> es({p.prevMO, p.e});
			std::vector<int> before2 = g.getPorfBefore(es);
			g.revisit.removePorfBefore(before2);

			// llvm::dbgs() << "After restriction: \n" << g << "\n";

			EventLabel &wLab = g.getEventLabel(p.e); /* Refetching */
			std::vector<Event> ls = g.getRevisitLoadsNonMaximal(p.e);
			std::vector<std::vector<Event > > rSets =
				EE->calcRevisitSets(ls, {}, wLab);
			revisitReads(g, rSets, {}, wLab);
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

	g.workqueue.pop_back();
	goto start;
}

void RCMCDriver::visitStore(ExecutionGraph &g)
{
	switch (userConf->model) {
	case wrc11:
		visitStoreWeakRA(g);
		return;
	default:
		visitStoreMO(g);
		return;
	}
}

void RCMCDriver::visitStoreWeakRA(ExecutionGraph &g)
{
	Event s = g.getLastThreadEvent(g.currentT);
	EventLabel &sLab = g.getEventLabel(s);

	g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);
	std::vector<Event> ls = g.getRevisitLoads(s);
	std::vector<std::vector<Event > > rSets =
		EE->calcRevisitSets(ls, {}, sLab);
	revisitReads(g, rSets, {}, sLab);
}

void RCMCDriver::visitStoreMO(ExecutionGraph &g)
{
	Event s = g.getLastThreadEvent(g.currentT);
	EventLabel &sLab = g.getEventLabel(s);

	View before = g.getHbBefore(s);
	std::vector<Event> locMO = g.modOrder.getAtLoc(sLab.addr);
	std::vector<std::vector<Event> > partStores = g.splitLocMOBefore(before, locMO);
	if (partStores[0].empty()) {
		g.modOrder.addAtLocEnd(sLab.addr, sLab.pos);

		std::vector<Event> ls = g.getRevisitLoads(s);
		std::vector<std::vector<Event > > rSets =
			EE->calcRevisitSets(ls, {}, sLab);
		revisitReads(g, rSets, {}, sLab);
	} else {
		std::vector<int> preds = g.getGraphState();
		RevisitSet prevRev = g.revisit;

		g.modOrder.addAtLocEnd(sLab.addr, s);

		std::vector<Event> ls = g.getRevisitLoads(s);
		std::vector<std::vector<Event > > rSets =
			EE->calcRevisitSets(ls, {}, sLab);
		revisitReads(g, rSets, {}, sLab);

		std::reverse(partStores[1].begin(), partStores[1].end());
		std::reverse(partStores[0].begin(), partStores[0].end());
		for (auto it = partStores[0].begin(); it != partStores[0].end(); ++it) {
			EventLabel &lab = g.getEventLabel(*it);
			if (lab.isRMW())
				continue;
			std::vector<Event> locMO(partStores[1]);
			locMO.insert(locMO.end(), partStores[0].begin(), it);
			locMO.push_back(s);
			locMO.insert(locMO.end(), it, partStores[0].end());

			g.workqueue.push_back(StackItem(s, *it, preds, prevRev, lab.addr, locMO));
		}
	}
	return;
}

bool RCMCDriver::visitRMWStore(ExecutionGraph &g)
{
	switch (userConf->model) {
	case wrc11:
		return visitRMWStoreWeakRA(g);
	default:
		return visitRMWStoreMO(g);
	}
}

bool RCMCDriver::visitRMWStoreWeakRA(ExecutionGraph &g)
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

bool RCMCDriver::visitRMWStoreMO(ExecutionGraph &g)
{
	Event s = g.getLastThreadEvent(g.currentT);
	EventLabel &sLab = g.getEventLabel(s);
	EventLabel &lab = g.getPreviousLabel(s); /* Need to refetch! */

	std::vector<Event> ls;
	std::vector<Event>::iterator pos = g.modOrder.getRMWPos(lab.addr, lab.rf);
	if (pos == g.modOrder.getAtLoc(lab.addr).end()) {
		g.modOrder.addAtLocEnd(lab.addr, s);
		ls = g.getRevisitLoads(s);
	} else {
		g.modOrder.addAtLocPos(lab.addr, pos, s);
		ls = g.getRevisitLoadsNonMaximal(s);
	}

	llvm::GenericValue val =
		EE->loadValueFromWrite(lab.rf, lab.valTyp, lab.addr);
	std::vector<Event> pendingRMWs =
		EE->getPendingRMWs(lab.pos, lab.rf, lab.addr, val);
	std::vector<std::vector<Event> > rSets =
		EE->calcRevisitSets(ls, pendingRMWs, sLab);

	revisitReads(g, rSets, pendingRMWs, sLab);

	return pendingRMWs.empty();
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

		ExecutionGraph eg;

		after = g.getPorfAfter(ls1);
		eg.cutToCopyAfter(g, after);
		eg.modifyRfs(ls1, wLab.pos);
		std::vector<int> before = eg.getPorfBefore(si);
		eg.revisit.removePorfBefore(before);

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
/* TODO: Fix destructors for Driver and config (basically for every class) */
