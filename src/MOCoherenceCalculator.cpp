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

#include "MOCoherenceCalculator.hpp"

int MOCoherenceCalculator::getStoreOffset(const llvm::GenericValue *addr, Event e) const
{
	BUG_ON(mo_.count(addr) == 0);

	if (e == Event::getInitializer())
		return -1;

	auto &locMO = mo_.at(addr);
	for (auto it = locMO.begin(); it != locMO.end(); ++it) {
		if (*it == e)
			return std::distance(locMO.begin(), it);
	}
	BUG();
}

std::pair<int, int>
MOCoherenceCalculator::getPossiblePlacings(const llvm::GenericValue *addr,
					   Event store, bool isRMW)
{
	const auto &g = getGraph();

	/* If it is an RMW store, there is only one possible position in MO */
	if (isRMW) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(g.getEventLabel(store))) {
			auto offset = getStoreOffset(addr, rLab->getRf()) + 1;
			return std::make_pair(offset, offset);
		}
		BUG();
	}

	/* Otherwise, we calculate the full range and add the store */
	auto rangeBegin = splitLocMOBefore(addr, store);
	auto rangeEnd = (supportsOutOfOrder()) ? splitLocMOAfter(addr, store) :
		getStoresToLoc(addr).size();
	return std::make_pair(rangeBegin, rangeEnd);

}

void MOCoherenceCalculator::addStoreToLoc(const llvm::GenericValue *addr,
					  Event store, int offset)
{
	mo_[addr].insert(mo_[addr].begin() + offset, store);
}

void MOCoherenceCalculator::changeStoreOffset(const llvm::GenericValue *addr,
					      Event store, int newOffset)
{
	auto &locMO = mo_[addr];

	locMO.erase(std::find(locMO.begin(), locMO.end(), store));
	locMO.insert(locMO.begin() + newOffset, store);
}

const std::vector<Event>&
MOCoherenceCalculator::getStoresToLoc(const llvm::GenericValue *addr)
{
	return mo_[addr];
}

int MOCoherenceCalculator::splitLocMOBefore(const llvm::GenericValue *addr,
					    Event e)

{
	const auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);
	auto &before = g.getHbBefore(e.prev());

	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (before.empty() || !g.isWriteRfBefore(before, *rit))
			continue;
		return std::distance(rit, locMO.rend());
	}
	return 0;
}

int MOCoherenceCalculator::splitLocMOAfterHb(const llvm::GenericValue *addr,
					     const Event read)
{
	const auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);

	auto initRfs = g.getInitRfsAtLoc(addr);
	for (auto &rf : initRfs) {
		if (g.getHbBefore(rf).contains(read))
			return 0;
	}

	for (auto it = locMO.begin(); it != locMO.end(); ++it) {
		if (g.isHbOptRfBefore(read, *it)) {
			if (g.getHbBefore(*it).contains(read))
				return std::distance(locMO.begin(), it);
			else
				return std::distance(locMO.begin(), it) + 1;
		}
	}
	return locMO.size();
}

int MOCoherenceCalculator::splitLocMOAfter(const llvm::GenericValue *addr,
					   const Event e)
{
	const auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);

	for (auto it = locMO.begin(); it != locMO.end(); ++it) {
		if (g.isHbOptRfBefore(e, *it))
			return std::distance(locMO.begin(), it);
	}
	return locMO.size();
}

std::vector<Event>
MOCoherenceCalculator::getCoherentStores(const llvm::GenericValue *addr,
					 Event read)
{
	auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);
	std::vector<Event> stores;

	/*
	 * If there are no stores (rf?;hb)-before the current event
	 * then we can read read from all concurrent stores and the
	 * initializer store. Otherwise, we can read from all concurrent
	 * stores and the mo-latest of the (rf?;hb)-before stores.
	 */
	auto begO = splitLocMOBefore(addr, read);
	if (begO == 0)
		stores.push_back(Event::getInitializer());
	else
		stores.push_back(*(locMO.begin() + begO - 1));

	/*
	 * If the model supports out-of-order execution we have to also
	 * account for the possibility the read is hb-before some other
	 * store, or some read that reads from a store.
	 */
	auto endO = (supportsOutOfOrder()) ? splitLocMOAfterHb(addr, read) :
		std::distance(locMO.begin(), locMO.end());
	stores.insert(stores.end(), locMO.begin() + begO, locMO.begin() + endO);
	return stores;
}

void MOCoherenceCalculator::removeStoresAfter(VectorClock &preds)
{
	for (auto it = mo_.begin(); it != mo_.end(); ++it)
		it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
						[&](Event &e)
						{ return !preds.contains(e); }),
				 it->second.end());
}
