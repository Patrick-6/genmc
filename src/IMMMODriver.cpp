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

#include "IMMMODriver.hpp"

/************************************************************
 ** MO DRIVER
 ***********************************************************/

void IMMMODriver::updateGraphDependencies(Event e,
					  const DepInfo &addr,
					  const DepInfo &data,
					  const DepInfo &ctrl,
					  const DepInfo &addrPo,
					  const DepInfo &casDep)
{
	auto &g = getGraph();

	g.addrDeps[e] = addr;
	g.dataDeps[e] = data;
	g.ctrlDeps[e] = ctrl;
	g.addrPoDeps[e] = addrPo;
	g.casDeps[e] = casDep;
}

void IMMMODriver::restrictGraph(unsigned int stamp)
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

int IMMMODriver::splitLocMOBefore(const llvm::GenericValue *addr, const View &before)
{
	auto &g = getGraph();
	auto &locMO = g.modOrder[addr];

	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (before.empty() || !g.isWriteRfBefore(before, *rit))
			continue;
		return std::distance(rit, locMO.rend());
	}
	return 0;
}

int IMMMODriver::splitLocMOAfter(const llvm::GenericValue *addr, const Event e)
{
	auto &g = getGraph();
	auto &locMO = g.modOrder[addr];

	for (auto it = locMO.begin(); it != locMO.end(); ++it) {
		if (g.isHbOptRfBefore(e, *it))
			return std::distance(locMO.begin(), it);
	}
	return locMO.size();
}

int IMMMODriver::splitLocMOAfterHb(const llvm::GenericValue *addr, const Event e)
{
	auto &g = getGraph();
	auto &locMO = g.modOrder[addr];

	auto initRfs = g.getInitRfsAtLoc(addr);
	for (auto &rf : initRfs) {
		if (g.getHbBefore(rf).contains(e))
			return 0;
	}

	for (auto it = locMO.begin(); it != locMO.end(); ++it) {
		if (g.isHbOptRfBefore(e, *it)) {
			if (g.getHbBefore(*it).contains(e))
				return std::distance(locMO.begin(), it);
			else
				return std::distance(locMO.begin(), it) + 1;
		}
	}
	return locMO.size();
}

std::vector<Event> IMMMODriver::getStoresToLoc(const llvm::GenericValue *addr)
{
	std::vector<Event> stores;

	auto &g = getGraph();
	auto &thr = getEE()->getCurThr();
	auto &locMO = g.modOrder[addr];
	auto last = Event(thr.id, thr.globalInstructions - 1);
	auto &before = g.getHbBefore(last);

	auto begO = splitLocMOBefore(addr, before);
	auto endO = splitLocMOAfterHb(addr, last.next());

	/*
	 * If there are no stores (hb;rf?)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (hb;rf?)-before stores.
	 */
	if (begO == 0)
		stores.push_back(Event::getInitializer());
	else
		stores.push_back(*(locMO.begin() + begO - 1));
	// if (endO != locMO.size() && !g.getHbBefore(*(locMO.begin() + endO)).contains(last.next()))
	// 	stores.insert(stores.end(), locMO.begin() + begO, locMO.begin() + endO + 1);
	// else
	stores.insert(stores.end(), locMO.begin() + begO, locMO.begin() + endO);
	return stores;
}

std::pair<int, int> IMMMODriver::getPossibleMOPlaces(const llvm::GenericValue *addr, bool isRMW)
{
	auto &g = getGraph();
	auto &thr = getEE()->getCurThr();
	Event curr = Event(thr.id, thr.globalInstructions - 1);

	if (isRMW) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(g.getEventLabel(curr))) {
			auto offset = g.modOrder.getStoreOffset(addr, rLab->getRf()) + 1;
			return std::make_pair(offset, offset);
		}
		BUG();
	}

	auto &before = g.getHbBefore(curr);
	return std::make_pair(splitLocMOBefore(addr, before),
			      splitLocMOAfter(addr, curr.next()));
}

std::vector<Event> IMMMODriver::getRevisitLoads(const WriteLabel *sLab)
{
	auto &g = getGraph();
	auto ls = g.getRevisitablePPoRf(sLab);
	auto &locMO = g.modOrder[sLab->getAddr()];

	// /* If this store is mo-maximal then we are done */
	// if (locMO.back() == sLab->getPos())
	// 	return ls;

	/* Otherwise, we have to exclude (mo;rf?;hb?;sb)-after reads */
	auto moOptRfs = g.getMoOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ const View &before = g.getHbPoBefore(e);
				  return std::any_of(moOptRfs.begin(), moOptRfs.end(),
					 [&](Event ev)
					 { return before.contains(ev); });
				}), ls.end());

	/* ...and we also have to exclude (mo^-1; rf?; hb?; sb)-before reads in
	 * the resulting graph */
	auto before = g.getPPoRfBefore(sLab->getPos());
	auto moInvOptRfs = g.getMoInvOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ auto *eLab = g.getEventLabel(e);
				  auto v = g.getDepViewFromStamp(eLab->getStamp());
				  v.update(before);
				  return std::any_of(moInvOptRfs.begin(),
						     moInvOptRfs.end(),
						     [&](Event ev)
						     { return v.contains(ev) &&
						       g.getHbPoBefore(ev).contains(e); });
				}),
		 ls.end());

	return ls;
}

std::pair<std::vector<std::unique_ptr<EventLabel> >,
	  std::vector<std::pair<Event, Event> > >
IMMMODriver::getPrefixToSaveNotBefore(const WriteLabel *sLab, const ReadLabel *rLab)
{
	auto &g = getGraph();
	auto prefix = g.getPrefixLabelsNotBeforePPoRf(sLab, rLab);
	auto moPlacings = g.getMOPredsInBefore(prefix, g.getDepViewFromStamp(rLab->getStamp()));

	BUG_ON(prefix.empty());
	return std::make_pair(std::move(prefix), std::move(moPlacings));
}

bool IMMMODriver::checkPscAcyclicity()
{
	switch (getConf()->checkPscAcyclicity) {
	case CheckPSCType::nocheck:
		return true;
	case CheckPSCType::weak:
	case CheckPSCType::wb:
		WARN_ONCE("check-mo-psc", "WARNING: The full PSC condition is going "
			  "to be checked for the MO-tracking exploration...\n");
	case CheckPSCType::full:
		return getGraph().isPscAcyclicMO();
	default:
		WARN("Unimplemented model!\n");
		BUG();
	}
}

bool IMMMODriver::isExecutionValid()
{
	return getGraph().isPscAcyclicMO();
}
