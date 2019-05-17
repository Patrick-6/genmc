/*
 * GenMC -- Generic Model Checking.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#include "IMMWBDriver.hpp"

void IMMWBDriver::updateGraphDependencies(Event e,
					  DepInfo &&addr,
					  DepInfo &&data,
					  DepInfo &&ctrl)
{
	auto &g = getGraph();

	g.addrDeps[e] = std::move(addr);
	g.dataDeps[e] = std::move(data);
	g.ctrlDeps[e] = std::move(ctrl);
}

void IMMWBDriver::restrictGraph(unsigned int stamp)
{
	auto &g = getGraph();

	/* First, free memory allocated by events that will no longer
	 * be in the graph */
	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = 1u; j < g.getThreadSize(i); j++) {
			const EventLabel *lab = g.getEventLabel(Event(i, j));
			if (lab->getStamp() <= stamp)
				continue;
			if (auto *mLab = llvm::dyn_cast<MallocLabel>(lab))
				getEE()->freeRegion(mLab->getAllocAddr(),
					       mLab->getAllocSize());
		}
	}

	/* Then, restrict the graph */
	g.cutToStamp(stamp);
	return;
}

std::vector<Event> IMMWBDriver::getStoresToLoc(const llvm::GenericValue *addr)
{
	auto &g = getGraph();
	auto &thr = getEE()->getCurThr();
	auto &hbBefore = g.getHbBefore(g.getLastThreadEvent(thr.id));
	auto wb = g.calcWb(addr);
	auto &stores = wb.getElems();
	std::vector<Event> result;

	/* Find the stores from which we can read-from */
	for (auto i = 0u; i < stores.size(); i++) {
		auto allowed = true;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wb(i, j) && g.isWriteRfBefore(hbBefore, stores[j])) {
				allowed = false;
				break;
			}
		}
		if (allowed)
			result.push_back(stores[i]);
	}

	/* Also check the initializer event */
	bool allowed = true;
	for (auto j = 0u; j < stores.size(); j++)
		if (g.isWriteRfBefore(hbBefore, stores[j]))
			allowed = false;
	if (allowed)
		result.insert(result.begin(), Event::getInitializer());
	return result;


	return result;
}

std::pair<int, int> IMMWBDriver::getPossibleMOPlaces(const llvm::GenericValue *addr, bool isRMW)
{
	auto locMOSize = (int) getGraph().modOrder[addr].size();
	return std::make_pair(locMOSize, locMOSize);
}

std::vector<Event> IMMWBDriver::getRevisitLoads(const WriteLabel *sLab)
{
	auto &g = getGraph();
	auto ls = g.getRevisitablePPoRf(sLab);
	llvm::dbgs() << "REVISITABLE FROM " << sLab->getPos() << ": ";
	for (auto &l :ls)
		llvm::dbgs() << l << " "; llvm::dbgs() << "\n";
	return ls;
}

std::pair<std::vector<std::unique_ptr<EventLabel> >,
	  std::vector<std::pair<Event, Event> > >
IMMWBDriver::getPrefixToSaveNotBefore(const WriteLabel *sLab, View &before)
{
	auto &g = getGraph();
	auto prefix = g.getPrefixLabelsNotBeforePPoRf(sLab, before);

	llvm::dbgs() << "COLLECTED PREFIX: ";
	for (auto &p : prefix)
		llvm::dbgs() << *p << "\n";
	return std::make_pair(std::move(prefix), std::vector<std::pair<Event, Event>>());
}

bool IMMWBDriver::checkPscAcyclicity()
{
	switch (getConf()->checkPscAcyclicity) {
	case CheckPSCType::nocheck:
		return true;
	case CheckPSCType::weak:
		return getGraph().isPscWeakAcyclicWB();
	case CheckPSCType::wb:
		return getGraph().isPscWbAcyclicWB();
	case CheckPSCType::full:
		return getGraph().isPscAcyclicWB();
	default:
		WARN("Unimplemented model!\n");
		BUG();
	}
}

bool IMMWBDriver::isExecutionValid()
{
	BUG();
}
