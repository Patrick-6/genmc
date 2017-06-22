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
#include "LLVMModule.hpp"
#include "RCMCDriver.hpp"
#include "Interpreter.h"
#include <llvm/IR/Verifier.h>

#include <algorithm>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

void abortHandler(int signum)
{
	exit(42);
}

RCMCDriver::RCMCDriver(Config *conf, clock_t start)
	: userConf(conf), explored(0), duplicates(0), start(start) {}
RCMCDriver::RCMCDriver(Config *conf, std::unique_ptr<llvm::Module> mod, clock_t start)
	: userConf(conf), mod(std::move(mod)), explored(0), duplicates(0), start(start) {}

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

 std::vector<std::vector<llvm::ExecutionContext> > initStacks;

 bool shouldContinue;
 bool executionCompleted = false;
 bool globalAccess = false;
bool interpStore = false;
bool interpRMW = false;

/* TODO: Move this to Interpreter.h, and also remove the relevant header */
 std::unordered_set<llvm::GenericValue *> globalVars;
 std::unordered_set<std::string> uniqueExecs;

void RCMCDriver::printResults()
{
	std::stringstream dups;
	dups << " (" << duplicates << " duplicates)";
	std::cerr << "Number of complete executions explored: " << explored
		  << ((userConf->countDuplicateExecs) ? dups.str() : "")
		  << std::endl;
	std::cerr << "Total wall-clock time: " << std::fixed << std::setprecision(2)
		  << ((float) clock() - start)/CLOCKS_PER_SEC
		  << "s" << std::endl;
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

	/* Get main program function and run the program */
	EE->runStaticConstructorsDestructors(false);
	EE->runFunctionAsMain(mod->getFunction("main"), {"prog"}, 0);
	EE->runStaticConstructorsDestructors(true);

	dryRun = false;
	/* There is not an event for any thread -- yet */
	/* maxEvents points to the array index of the next event */
	for (int i = 0; i < initNumThreads; i++) {
		Thread &t = initGraph.threads[i];
		t.eventList.push_back(EventLabel(NA, Event(-1, -1)));
		initGraph.maxEvents.push_back(1);
	}

	visitGraph(initGraph);
	printResults();
	return;
}

void RCMCDriver::visitGraph(ExecutionGraph &g)
{
	ExecutionGraph *oldEG = currentEG;
	bool oldContinue = shouldContinue;
	bool oldCompleted = executionCompleted;
	currentEG = &g;
	for (int i = 0; i < initNumThreads; i++) {
		g.threads[i].ECStack = initStacks[i];
	}

	while (true) {
		interpStore = false;
		interpRMW = false;
		if (userConf->validateExecGraphs)
			g.validateGraph();
		if (g.scheduleNext()) {
			shouldContinue = true;
			executionCompleted = false;
			llvm::ExecutionContext &SF = g.getThreadECStack(g.currentT).back();
			llvm::Instruction &I = *SF.CurInst++;
			EE->visit(I);
			if (interpStore) {
				Event s = g.getLastThreadEvent(g.currentT);
				EventLabel &sLab = g.getEventLabel(s);

				std::vector<Event> ls = g.getRevisitLoads(s);
				std::vector<std::vector<Event > > rSets =
					EE->calcRevisitSets(ls, {}, sLab);
				revisitReads(g, rSets, {}, sLab);
			} else if (interpRMW) {
				Event s = g.getLastThreadEvent(g.currentT);
				EventLabel &sLab = g.getEventLabel(s);
				EventLabel &lab = g.getPreviousLabel(s); /* Need to refetch! */
				std::vector<Event> ls = g.getRevisitLoads(s);
				llvm::Type *typ;
				if (llvm::isa<llvm::AtomicCmpXchgInst>(I))
					typ = llvm::cast<llvm::AtomicCmpXchgInst>(I).getCompareOperand()->getType();
				else
					typ = I.getType();
				llvm::GenericValue val =
					EE->loadValueFromWrite(lab.rf, typ, lab.addr);
				std::vector<Event> pendingRMWs =
					EE->getPendingRMWs(lab.pos, lab.rf, lab.addr, val);
				std::vector<std::vector<Event> > rSets =
					EE->calcRevisitSets(ls, pendingRMWs, sLab);

				revisitReads(g, rSets, pendingRMWs, sLab);
				if (!pendingRMWs.empty())
					shouldContinue = false;
			}
		} else if (std::any_of(g.threads.begin(), g.threads.end(),
				       [](Thread &thr){ return thr.isBlocked; })) {
			executionCompleted = true;
		} else {
			if (userConf->printExecGraphs)
				std::cerr << g << std::endl;
			if (userConf->countDuplicateExecs) {
				std::stringstream buf;
				buf << g;
				if (uniqueExecs.find(buf.str()) != uniqueExecs.end())
					++duplicates;
				else
					uniqueExecs.insert(buf.str());
			}
			++explored;
			executionCompleted = true;
		}

		if (shouldContinue && !executionCompleted)
			continue;

		if (g.workqueue.empty()) {
			for (auto mem : g.stackAllocas)
				free(mem); /* No need to clear vector */
			for (auto mem : g.heapAllocas)
				free(mem);
			shouldContinue = oldContinue;
			executionCompleted = oldCompleted;
			currentEG = oldEG;
			return;
		}

		StackItem &p = g.workqueue.back();
		// std::cerr << "Popping from workqueue " << p.e << " should rf "
		// 	  << p.shouldRf << "\n" << g << std::endl;

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

		std::vector<int> before = g.getPorfBefore(p.e);
		for (auto it = g.revisit.begin(); it != g.revisit.end(); ++it)
			if (it->first.index <= before[it->first.thread])
				g.revisit.erase(it--);
		before = g.getPorfBefore(p.rf);
		for (auto it = g.revisit.begin(); it != g.revisit.end(); ++it)
			if (it->first.index <= before[it->first.thread])
				it->second.push_back(p.e);
//		std::cerr << "After restriction: \n" << g << std::endl;

		for (int i = 0; i < initNumThreads; i++) {
			g.threads[i].ECStack = initStacks[i];
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
	}
}

void RCMCDriver::revisitReads(ExecutionGraph &g, std::vector<std::vector<Event> > &subsets,
			      std::vector<Event> K0, EventLabel &wLab)
{
	if (!K0.empty()) {
		std::vector<int> before = g.getPorfBefore(wLab.pos);
		for (auto &r : g.revisit) {
			if (K0.back() != r.first)
				continue;
			if (std::any_of(r.second.begin(), r.second.end(),
					[&before](Event &e)
					{ return e.index > before[e.thread]; }))
				return;
		}
	}

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
		after = g.getPorfAfter(ls1);

		ExecutionGraph eg;

		eg.cutToCopyAfter(g, after);
		eg.modifyRfs(ls1, wLab.pos);
		std::vector<int> before = eg.getPorfBefore(si);
		for (auto it = eg.revisit.begin(); it != eg.revisit.end(); ++it) {
			if (it->first.index <= before[it->first.thread])
				eg.revisit.erase(it--);
			else
				it->second.erase(std::remove_if(it->second.begin(),
						 it->second.end(), [&before](Event &e)
						 { return e.index <= before[e.thread]; }),
						 it->second.end());
		}
		for (auto it = eg.revisit.begin(); it != eg.revisit.end(); ++it) {
			after = g.getPorfAfter(it->first);
			for (auto &s : si)
				if (s.index >= after[s.thread])
					it->second.push_back(s);
		}
		visitGraph(eg);
	}
	return;
}

/* TODO: Fix destructors for Driver and config (basically for every class) */
