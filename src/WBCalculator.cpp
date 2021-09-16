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

#include "WBCalculator.hpp"
#include "WBIterator.hpp"

/*
 * Given a WB matrix returns a vector that, for each store in the WB
 * matrix, contains the index (in the WB matrix) of the upper and the
 * lower limit of the RMW chain that store belongs to. We use N for as
 * the index of the initializer write, where N is the number of stores
 * in the WB matrix.
 *
 * The vector is partitioned into 3 parts: | UPPER | LOWER | LOWER_I |
 *
 * The first part contains the upper limits, the second part the lower
 * limits and the last part the lower limit for the initiazizer write,
 * which is not part of the WB matrix.
 *
 * If there is an atomicity violation in the graph, the returned
 * vector is empty. (This may happen as part of some optimization,
 * e.g., in getRevisitLoads(), where the view of the resulting graph
 * is not calculated.)
 */
std::vector<unsigned int> WBCalculator::calcRMWLimits(const GlobalRelation &wb) const
{
	auto &g = getGraph();
	auto &s = wb.getElems();
	auto size = s.size();

	/* upperL is the vector to return, with size (2 * N + 1) */
	std::vector<unsigned int> upperL(2 * size + 1);
	std::vector<unsigned int>::iterator lowerL = upperL.begin() + size;

	/*
	 * First, we initialize the vector. For the upper limit of a
	 * non-RMW store we set the store itself, and for an RMW-store
	 * its predecessor in the RMW chain. For the lower limit of
	 * any store we set the store itself.  For the initializer
	 * write, we use "size" as its index, since it does not exist
	 * in the WB matrix.
	 */
	for (auto i = 0u; i < size; i++) {
		/* static casts below are based on the construction of the EG */
		auto *wLab = static_cast<const WriteLabel *>(g.getEventLabel(s[i]));
		if (llvm::isa<FaiWriteLabel>(wLab) || llvm::isa<CasWriteLabel>(wLab)) {
			auto *pLab = static_cast<const ReadLabel *>(
				g.getPreviousLabel(wLab));
			Event prev = pLab->getRf();
			upperL[i] = (prev == Event::getInitializer()) ? size :
				wb.getIndex(prev);
		} else {
			upperL[i] = i;
		}
		lowerL[i] = i;
	}
	lowerL[size] = size;

	/*
	 * Next, we set the lower limit of the upper limit of an RMW
	 * store to be the store itself.
	 */
	for (auto i = 0u; i < size; i++) {
		auto ui = upperL[i];
		if (ui == i)
			continue;
		if (lowerL[ui] != ui) {
			/* If the lower limit of this upper limit has already
			 * been set, we have two RMWs reading from the same write */
			upperL.clear();
			return upperL;
		}
		lowerL[upperL[i]] = i;
	}

	/*
	 * Calculate the actual upper limit, by taking the
	 * predecessor's predecessor as an upper limit, until the end
	 * of the chain is reached.
	 */
        bool changed;
	do {
		changed = false;
		for (auto i = 0u; i < size; i++) {
			auto j = upperL[i];
			if (j == size || j == i)
				continue;

			auto k = upperL[j];
			if (j == k)
				continue;
			upperL[i] = k;
			changed = true;
		}
	} while (changed);

	/* Similarly for the lower limits */
	do {
		changed = false;
		for (auto i = 0u; i <= size; i++) {
			auto j = lowerL[i];
			if (j == i)
				continue;
			auto k = lowerL[j];
			if (j == k)
				continue;
			lowerL[i] = k;
			changed = true;
		}
	} while (changed);
	return upperL;
}

Calculator::GlobalRelation
WBCalculator::calcWbRestricted(SAddr addr, const VectorClock &v) const
{
	auto &g = getGraph();
	auto &locMO = getStoresToLoc(addr);
	std::vector<Event> storesInView;

	std::copy_if(locMO.begin(), locMO.end(), std::back_inserter(storesInView),
		     [&](const Event &s){ return v.contains(s); });

	GlobalRelation matrix(std::move(storesInView));
	auto &stores = matrix.getElems();

	/* Optimization */
	if (stores.size() <= 1)
		return matrix;

	auto upperLimit = calcRMWLimits(matrix);
	if (upperLimit.empty()) {
		for (auto i = 0u; i < stores.size(); i++)
			matrix.addEdge(i,i);
		return matrix;
	}

	auto lowerLimit = upperLimit.begin() + stores.size();

	for (auto i = 0u; i < stores.size(); i++) {
		auto *wLab = static_cast<const WriteLabel *>(g.getEventLabel(stores[i]));

		std::vector<Event> es;
		const std::vector<Event> readers = wLab->getReadersList();
		std::copy_if(readers.begin(), readers.end(), std::back_inserter(es),
			     [&](const Event &r){ return v.contains(r); });

		es.push_back(wLab->getPos());
		auto upi = upperLimit[i];
		for (auto j = 0u; j < stores.size(); j++) {
			if (i == j || std::none_of(es.begin(), es.end(), [&](Event e)
				      { return g.isWriteRfBefore(stores[j], e); }))
				continue;
			matrix.addEdge(j, i);

			if (upi == stores.size() || upi == upperLimit[j])
				continue;
			matrix.addEdge(lowerLimit[j], upi);
		}

		if (lowerLimit[stores.size()] == stores.size() || upi == stores.size())
			continue;
		matrix.addEdge(lowerLimit[stores.size()], i);
	}
	matrix.transClosure();
	return matrix;
}

Calculator::GlobalRelation WBCalculator::calcWb(SAddr addr) const
{
	auto &g = getGraph();
	GlobalRelation matrix(getStoresToLoc(addr));
	auto &stores = matrix.getElems();

	/* Optimization */
	if (stores.size() <= 1)
		return matrix;

	auto upperLimit = calcRMWLimits(matrix);
	if (upperLimit.empty()) {
		for (auto i = 0u; i < stores.size(); i++)
			matrix.addEdge(i, i);
		return matrix;
	}

	auto lowerLimit = upperLimit.begin() + stores.size();

	for (auto i = 0u; i < stores.size(); i++) {
		auto *wLab = static_cast<const WriteLabel *>(g.getEventLabel(stores[i]));
		std::vector<Event> es(wLab->getReadersList());
		es.push_back(wLab->getPos());

		auto upi = upperLimit[i];
		for (auto j = 0u; j < stores.size(); j++) {
			if (i == j || std::none_of(es.begin(), es.end(), [&](Event e)
				      { return g.isWriteRfBefore(stores[j], e); }))
				continue;
			matrix.addEdge(j, i);
			if (upi == stores.size() || upi == upperLimit[j])
				continue;
			matrix.addEdge(lowerLimit[j], upi);
		}

		if (lowerLimit[stores.size()] == stores.size() || upi == stores.size())
			continue;
		matrix.addEdge(lowerLimit[stores.size()], i);
	}
	matrix.transClosure();
	return matrix;
}

void WBCalculator::trackCoherenceAtLoc(SAddr addr)
{
	stores_[addr];
}


std::pair<int, int>
WBCalculator::getPossiblePlacings(SAddr addr, Event store, bool isRMW)
{
	auto locMOSize = getStoresToLoc(addr).size();
	return std::make_pair(locMOSize, locMOSize);
}

void WBCalculator::addStoreToLoc(SAddr addr, Event store, int offset)
{
	/* The offset given is ignored */
	stores_[addr].push_back(store);
}

void WBCalculator::addStoreToLocAfter(SAddr addr, Event store, Event pred)
{
	/* Again the offset given is ignored */
	addStoreToLoc(addr, store, 0);
}

bool WBCalculator::isCoMaximal(SAddr addr, Event store)
{
	auto &stores = getStoresToLoc(addr);
	if (store.isInitializer())
		return stores.empty();

	auto wb = calcWb(addr);
	return wb.adj_begin(store) == wb.adj_end(store);
}

bool WBCalculator::isCachedCoMaximal(SAddr addr, Event store)
{
	auto &stores = getStoresToLoc(addr);
	if (store.isInitializer())
		return stores.empty();

	return cache_.isMaximal(store);
}

const std::vector<Event>&
WBCalculator::getStoresToLoc(SAddr addr) const
{
	BUG_ON(stores_.count(addr) == 0);
	return stores_.at(addr);
}

/*
 * Checks which of the stores are (rf?;hb)-before some event e, given the
 * hb-before view of e
 */
View WBCalculator::getRfOptHbBeforeStores(const std::vector<Event> &stores, const View &hbBefore)
{
	const auto &g = getGraph();
	View result;

	for (const auto &w : stores) {
		/* Check if w itself is in the hb view */
		if (hbBefore.contains(w)) {
			result.updateIdx(w);
			continue;
		}

		const EventLabel *lab = g.getEventLabel(w);
		BUG_ON(!llvm::isa<WriteLabel>(lab));
		auto *wLab = static_cast<const WriteLabel *>(lab);

		/* Check whether [w];rf;[r] is in the hb view, for some r */
		for (const auto &r : wLab->getReadersList()) {
			if (r.thread != w.thread && hbBefore.contains(r)) {
				result.updateIdx(w);
				result.updateIdx(r);
				break;
			}
		}
	}
	return result;
}

void WBCalculator::expandMaximalAndMarkOverwritten(const std::vector<Event> &stores,
						   View &storeView)
{
	const auto &g = getGraph();

	/* Expand view for maximal stores */
	for (const auto &w : stores) {
		/* If the store is not maximal, skip */
		if (w.index != storeView[w.thread])
			continue;

		const EventLabel *lab = g.getEventLabel(w);
		BUG_ON(!llvm::isa<WriteLabel>(lab));
		auto *wLab = static_cast<const WriteLabel *>(lab);

		for (const auto &r : wLab->getReadersList()) {
			if (r.thread != w.thread)
				storeView.updateIdx(r);
		}
	}

	/* Check if maximal writes have been overwritten */
	for (const auto &w : stores) {
		/* If the store is not maximal, skip*/
		if (w.index != storeView[w.thread])
			continue;

		const EventLabel *lab = g.getEventLabel(w);
		BUG_ON(!llvm::isa<WriteLabel>(lab));
		auto *wLab = static_cast<const WriteLabel *>(lab);

		for (const auto &r : wLab->getReadersList()) {
			if (r.thread != w.thread && r.index < storeView[r.thread]) {
				const EventLabel *lab = g.getEventLabel(Event(r.thread, storeView[r.thread]));
				if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
					if (rLab->getRf() != w) {
						storeView[w.thread] = std::min(storeView[w.thread] + 1,
									       int(g.getThreadSize(w.thread)) - 1);
						break;
					}
				}
			}
		}
	}
	return;
}

bool WBCalculator::tryOptimizeWBCalculation(SAddr addr,
					    Event read,
					    std::vector<Event> &result)
{
	auto &g = getGraph();
	auto &allStores = getStoresToLoc(addr);
	auto &hbBefore = g.getEventLabel(read.prev())->getHbView();
	auto view = getRfOptHbBeforeStores(allStores, hbBefore);

	/* Can we read from the initializer event? */
	if (std::none_of(view.begin(), view.end(), [](int i){ return i > 0; }))
		result.push_back(Event::getInitializer());

	expandMaximalAndMarkOverwritten(allStores, view);

	int count = 0;
	for (const auto &w : allStores) {
		if (w.index >= view[w.thread]) {
			if (count++ > 0) {
				result.pop_back();
				break;
			}
			result.push_back(w);

			/* Also set the cache */
			BUG_ON(result.size() > 2);
			getCache().addMaximalInfo(result);
		}
	}
	return (count <= 1);
}

bool WBCalculator::isCoherentRf(SAddr addr,
				const GlobalRelation &wb,
				Event read, Event store, int storeWbIdx)
{
	auto &g = getGraph();
	auto &stores = wb.getElems();

	/* First, check whether it is wb;rf?;hb-before the read */
	for (auto j = 0u; j < stores.size(); j++) {
		if (wb(storeWbIdx, j) && g.isWriteRfBefore(stores[j], read.prev()))
			return false;
	}

	/* If OOO execution is _not_ supported no need to do extra checks */
	if (!supportsOutOfOrder())
		return true;

	/* For the OOO case, we have to do extra checks */
	for (auto j = 0u; j < stores.size(); j++) {
		if (wb(j, storeWbIdx) && g.isHbOptRfBefore(read, stores[j]))
			return false;
	}

	/* We cannot read from hb-after stores... */
	if (g.getEventLabel(store)->getHbView().contains(read))
		return false;

	/* Also check for violations against the initializer */
	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = 1u; j < g.getThreadSize(i); j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
				if (rLab->getRf().isInitializer() &&
				    rLab->getAddr() == addr &&
				    g.getEventLabel(rLab->getPos())->getHbView().contains(read))
					return false;
		}
	}
	return true;
}

bool WBCalculator::isInitCoherentRf(const GlobalRelation &wb, Event read)
{
	auto &g = getGraph();
	auto &stores = wb.getElems();

	for (auto j = 0u; j < stores.size(); j++)
		if (g.isWriteRfBefore(stores[j], read.prev()))
			return false;
	return true;
}

std::vector<Event>
WBCalculator::getCoherentStores(SAddr addr, Event read)
{
	const auto &g = getGraph();
	std::vector<Event> result;

	/*  For the in-order execution case:
	 *
	 * Check whether calculating WB is necessary. As a byproduct, if the
	 * initializer is a valid RF, it is pushed into result */
	if (!supportsOutOfOrder() && tryOptimizeWBCalculation(addr, read, result))
		return result;

	auto wb = calcWb(addr);
	auto &stores = wb.getElems();

	/* Find the stores from which we can read-from */
	for (auto i = 0u; i < stores.size(); i++) {
		if (isCoherentRf(addr, wb, read, stores[i], i))
			result.push_back(stores[i]);
	}

	/* Ensure function contract is satisfied */
	std::sort(result.begin(), result.end(), [&g, &wb](const Event &a, const Event &b){
		if (b.isInitializer())
			return false;
		return a.isInitializer() || wb(a, b) ||
			(!wb(b, a) && g.getEventLabel(a)->getStamp() < g.getEventLabel(b)->getStamp());
	});

	/* For the OOO execution case:
	 *
	 * We check whether the initializer is a valid RF as well */
	if (supportsOutOfOrder() && isInitCoherentRf(wb, read))
		result.insert(result.begin(), Event::getInitializer());

	/* Set the cache before returning */
	getCache().addCalcInfo(std::move(wb));
	return result;
}

bool WBCalculator::isWbMaximal(const WriteLabel *sLab, const std::vector<Event> &ls) const
{
	/* Optimization:
	 * Since sLab is a porf-maximal store, unless it is an RMW, it is
	 * wb-maximal (and so, all revisitable loads can read from it).
	 */
	if (!llvm::isa<FaiWriteLabel>(sLab) && !llvm::isa<CasWriteLabel>(sLab))
		return true;

	/* Optimization:
	 * If sLab is maximal in WB, then all revisitable loads can read
	 * from it.
	 */
	if (ls.size() > 1) {
		auto wb = calcWb(sLab->getAddr());
		auto i = wb.getIndex(sLab->getPos());
		bool allowed = true;
		for (auto j = 0u; j < wb.getElems().size(); j++)
			if (wb(i, j)) {
				return false;
			}
		return true;
	}
	return false;
}

bool WBCalculator::isCoherentRevisit(const WriteLabel *sLab, Event read) const
{
	auto &g = getGraph();
	const EventLabel *rLab = g.getEventLabel(read);

	BUG_ON(!llvm::isa<ReadLabel>(rLab));
	auto v = g.getRevisitView(static_cast<const ReadLabel *>(rLab), sLab);
	auto wb = calcWbRestricted(sLab->getAddr(), *v);
	auto &stores = wb.getElems();
	auto i = wb.getIndex(sLab->getPos());

	for (auto j = 0u; j < stores.size(); j++) {
		if (wb(i, j) && g.isWriteRfBefore(stores[j], g.getPreviousNonEmptyLabel(read)->getPos())) {
			return false;
		}
	}

	/* If OOO is _not_ supported, no more checks are required */
	if (!supportsOutOfOrder())
		return true;

	for (auto j = 0u; j < stores.size(); j++) {
		if (wb(j, i) && g.isHbOptRfBeforeInView(read, stores[j], *v))
				return false;
	}

	/* Do not revisit hb-before loads... */
	if (g.getEventLabel(sLab->getPos())->getHbView().contains(read))
		return false;

	/* Also check for violations against the initializer */
	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = 1u; j < g.getThreadSize(i); j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (lab->getPos() == read)
				continue;
			if (!v->contains(lab->getPos()))
				continue;
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
				if (rLab->getRf().isInitializer() &&
				    rLab->getAddr() == sLab->getAddr() &&
				    g.getEventLabel(rLab->getPos())->getHbView().contains(read))
					return false;
		}
	}
	return true;
}

std::vector<Event>
WBCalculator::getCoherentRevisits(const WriteLabel *sLab)
{
	const auto &g = getGraph();
	auto ls = g.getRevisitable(sLab);

	if (!supportsOutOfOrder() && isWbMaximal(sLab, ls))
		return ls;

	std::vector<Event> result;

	/*
	 * We calculate WB again, in order to filter-out inconsistent
	 * revisit options. For example, if sLab is an RMW, we cannot
	 * revisit a read r for which:
	 * \exists c_a in C_a .
	 *         (c_a, r) \in (hb;[\lW_x];\lRF^?;hb;po)
	 *
	 * since this will create a cycle in WB
	 */

	for (auto &l : ls) {
		if (isCoherentRevisit(sLab, l))
			result.push_back(l);
	}
	return result;
}

const Calculator::GlobalRelation &
WBCalculator::getOrInsertWbCalc(SAddr addr, const View &v, Calculator::PerLocRelation &cache)
{
	if (!cache.count(addr))
		cache[addr] = calcWbRestricted(addr, v);
	return cache.at(addr);
}

bool WBCalculator::isCoAfterRemoved(const ReadLabel *rLab, const WriteLabel *sLab,
				    const EventLabel *lab, Calculator::PerLocRelation &wbs)
{
	auto &g = getGraph();
	if (!llvm::isa<WriteLabel>(lab) || g.isRMWStore(lab))
		return false;

	auto *wLab = llvm::dyn_cast<WriteLabel>(lab);
	BUG_ON(!wLab);

	auto p = g.getPredsView(sLab->getPos());
	--(*p)[sLab->getThread()];

	auto &wb = getOrInsertWbCalc(wLab->getAddr(), *llvm::dyn_cast<View>(&*p), wbs);
	auto &stores = wb.getElems();
	return std::any_of(wb_pred_begin(wb, wLab->getPos()),
			   wb_pred_end(wb, wLab->getPos()), [&](const Event &s){
				   auto *slab = g.getEventLabel(s);
				   return g.revisitDeletesEvent(rLab, sLab, slab) &&
					   slab->getStamp() < wLab->getStamp() &&
					   !(g.isRMWStore(slab) && slab->getPos().prev() == rLab->getPos());
			   });
}

bool WBCalculator::isRbBeforeSavedPrefix(const ReadLabel *revLab, const WriteLabel *wLab,
					   const EventLabel *lab, Calculator::PerLocRelation &wbs)
{
	auto *rLab = llvm::dyn_cast<ReadLabel>(lab);
	if (!rLab)
		return false;

	auto &g = getGraph();
        auto &v = g.getPrefixView(wLab->getPos());
	auto p = g.getPredsView(wLab->getPos());
	--(*p)[wLab->getThread()];

	auto &wb = getOrInsertWbCalc(rLab->getAddr(), *llvm::dyn_cast<View>(&*p), wbs);
	auto &stores = wb.getElems();
	return std::any_of(wb_succ_begin(wb, rLab->getRf()),
			   wb_succ_end(wb, rLab->getRf()), [&](const Event &s){
				   auto *sLab = g.getEventLabel(s);
				   return v.contains(sLab->getPos()) && sLab->getPos() != wLab->getPos() &&
					   sLab->getStamp() > revLab->getStamp();
			   });
}

bool WBCalculator::coherenceSuccRemainInGraph(const ReadLabel *rLab, const WriteLabel *wLab)
{
	auto &g = getGraph();
	if (g.isRMWStore(wLab))
		return true;

	auto wb = calcWb(rLab->getAddr());
	auto &stores = wb.getElems();

	/* Find the "immediate" successor of wLab */
	std::vector<Event> succs;
	for (auto it = wb.adj_begin(wLab->getPos()), ie = wb.adj_end(wLab->getPos()); it != ie; ++it)
		succs.push_back(stores[*it]);
	if (succs.empty())
		return true;

	std::sort(succs.begin(), succs.end(), [&g](const Event &a, const Event &b)
		{ return g.getEventLabel(a) < g.getEventLabel(b); });
	auto *sLab = g.getEventLabel(succs[0]);
	return sLab->getStamp() <= rLab->getStamp() || g.getPrefixView(wLab->getPos()).contains(sLab->getPos());
}

Event WBCalculator::getOrInsertWbMaximal(SAddr addr, View &v, std::unordered_map<SAddr, Event> &cache)
{
	if (!cache.count(addr)) {
		auto &g = getGraph();

		auto wb = calcWbRestricted(addr, v);
		auto &stores = wb.getElems();

		/* It's a bit tricky to move this check above the if, due to atomicity violations */
		if (stores.empty())
			return Event::getInitializer();

		std::vector<Event> maximals;
		for (auto &s : stores) {
			if (wb.adj_begin(s) == wb.adj_end(s))
				maximals.push_back(s);
		}
		std::sort(maximals.begin(), maximals.end(), [&g](const Event &a, const Event &b)
			{ return g.getEventLabel(a)->getStamp() < g.getEventLabel(b)->getStamp(); });

		cache[addr] = maximals.back();
	}
	return cache.at(addr);
}

Event WBCalculator::getTiebraker(const ReadLabel *rLab, const WriteLabel *wLab, const ReadLabel *lab) const
{
	auto &g = getGraph();
	auto &locMO = getStoresToLoc(lab->getAddr());

	auto *tbLab = g.getEventLabel(lab->getRf());
	for (const auto &s : locMO) {
		auto *sLab = g.getEventLabel(s);
		if (g.revisitDeletesEvent(rLab, wLab, sLab) && sLab->getStamp() < lab->getStamp() &&
		    !llvm::isa<BIncFaiWriteLabel>(sLab) && sLab->getStamp() > tbLab->getStamp())
			tbLab = sLab;
	}
	return tbLab->getPos();
}

bool WBCalculator::ignoresDeletedStore(const ReadLabel *rLab, const WriteLabel *wLab, const ReadLabel *lab) const
{
	auto &g = getGraph();
	auto &locMO = getStoresToLoc(lab->getAddr());

	return std::any_of(locMO.begin(), locMO.end(), [&](const Event &s){
		auto *sLab = g.getEventLabel(s);
		return g.revisitDeletesEvent(rLab, wLab, sLab) && sLab->getStamp() < lab->getStamp();
	});
}

bool WBCalculator::wasAddedMaximally(const ReadLabel *rLab, const WriteLabel *wLab,
				     const EventLabel *eLab, std::unordered_map<SAddr, Event> &cache)
{
	auto *lab = llvm::dyn_cast<ReadLabel>(eLab);
	if (!lab)
		return true;

	auto &g = getGraph();
	auto p = g.getRevisitView(rLab, wLab);

	/* If it reads from the deleted events, it must be reading from the deleted event added latest before it */
	if (!p->contains(lab->getRf()))
		return lab->getRf() == getTiebraker(rLab, wLab, lab);

	/* If there is a store that will not remain in the graph (but added after RLAB),
	 * LAB should be reading from there */
	auto &locMO = getStoresToLoc(lab->getAddr());
	if (ignoresDeletedStore(rLab, wLab, lab))
		return false;

	/* Otherwise, it needs to be reading from the maximal write in the remaining graph */
	auto v = g.getRevisitView(rLab, wLab);
	BUG_ON(llvm::isa<DepView>(&*v));

	--(*v)[rLab->getThread()];
	--(*v)[wLab->getThread()];

	return lab->getRf() == getOrInsertWbMaximal(lab->getAddr(), *llvm::dyn_cast<View>(&*v), cache);
}

bool WBCalculator::inMaximalPath(const ReadLabel *rLab, const WriteLabel *wLab)
{
	if (!coherenceSuccRemainInGraph(rLab, wLab))
		return false;

	auto &g = getGraph();
        auto &v = g.getPrefixView(wLab->getPos());
	Calculator::PerLocRelation wbs;
	std::unordered_map<SAddr, Event> initMaximals;

	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = g.getThreadSize(i) - 1; j != 0u; j--) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (lab->getStamp() < rLab->getStamp())
				break;
			if (v.contains(lab->getPos())) {
				if (lab->getPos() != wLab->getPos() && isCoAfterRemoved(rLab, wLab, lab, wbs))
					return false;
				continue;
			}

			if (isRbBeforeSavedPrefix(rLab, wLab, lab, wbs)) {
				// llvm::dbgs() << "FR: invalid revisit " << rLab->getPos() << " from " << wLab->getPos();
				return false;
			}

			if (g.hasBeenRevisitedByDeleted(rLab, wLab, lab)) {
				// llvm::dbgs() << "RF WILL BE DELETED\n";
				return false;
			}

			if (!wasAddedMaximally(rLab, wLab, lab, initMaximals)) {
				// llvm::dbgs() << "INVALIDUE TO NON MAXIMALITY\n";
				// if (lab == rLab  && rfFromPrefix)
				// 	continue;
				// llvm::dbgs() << "cannot revisit " << rLab->getPos() << " from " << wLab->getPos();
				// llvm::dbgs() << " due to " << lab->getPos() << " in\n";
				// printGraph();
				return false;
			}
		}
	}
	return true;
}

std::vector<std::pair<Event, Event> >
WBCalculator::saveCoherenceStatus(const std::vector<std::unique_ptr<EventLabel> > &labs,
				  const ReadLabel *rLab) const
{
	std::vector<std::pair<Event, Event> > pairs;

	for (const auto &lab : labs) {
		/* Only store MO pairs for write labels */
		if (!llvm::isa<WriteLabel>(lab.get()))
			continue;

		auto *wLab = static_cast<const WriteLabel *>(lab.get());
		auto &locMO = getStoresToLoc(wLab->getAddr());
		auto moPos = std::find(locMO.begin(), locMO.end(), wLab->getPos());

		/* We are not actually saving anything, but we do need to make sure
		 * that the store is in this location's MO */
		BUG_ON(moPos == locMO.end());
	}
	return pairs;
}

void WBCalculator::initCalc()
{
	auto &g = getGraph();
	auto &coRelation = g.getPerLocRelation(ExecutionGraph::RelationId::co);

	coRelation.clear(); /* in case this is called directly (e.g., from PersChecker) */
	for (auto it = stores_.begin(); it != stores_.end(); ++it)
		coRelation[it->first] = calcWb(it->first);
	return;
}

Calculator::CalculationResult WBCalculator::doCalc()
{
	auto &g = getGraph();
	auto &hbRelation = g.getGlobalRelation(ExecutionGraph::RelationId::hb);
	auto &coRelation = g.getPerLocRelation(ExecutionGraph::RelationId::co);

	bool changed = false;
	for (auto locIt = stores_.begin(); locIt != stores_.end(); ++locIt) {
		auto &matrix = coRelation[locIt->first];
		auto &stores = matrix.getElems();

		/* If it is empty, nothing to do */
		if (stores.empty())
			continue;

		auto upperLimit = calcRMWLimits(matrix);
		if (upperLimit.empty()) {
			for (auto i = 0u; i < stores.size(); i++)
				matrix.addEdge(i, i);
			return Calculator::CalculationResult(true, false);
		}

		auto lowerLimit = upperLimit.begin() + stores.size();
		for (auto i = 0u; i < stores.size(); i++) {
			auto *wLab = static_cast<const WriteLabel *>(g.getEventLabel(stores[i]));
			std::vector<Event> es(wLab->getReadersList());
			es.push_back(wLab->getPos());

			auto upi = upperLimit[i];
			for (auto j = 0u; j < stores.size(); j++) {
				if (i == j ||
				    std::none_of(es.begin(), es.end(), [&](Event e)
						 { return g.isWriteRfBeforeRel(hbRelation, stores[j], e); }))
					continue;

				if (!matrix(j, i)) {
					changed = true;
					matrix.addEdge(j, i);
				}
				if (upi == stores.size() || upi == upperLimit[j])
					continue;

				if (!matrix(lowerLimit[j], upi)) {
					matrix.addEdge(lowerLimit[j], upi);
					changed = true;
				}
			}

			if (lowerLimit[stores.size()] == stores.size() || upi == stores.size())
				continue;

			if (!matrix(lowerLimit[stores.size()], i)) {
				matrix.addEdge(lowerLimit[stores.size()], i);
				changed = true;
			}
		}
		matrix.transClosure();

		/* Check for consistency */
		if (!matrix.isIrreflexive())
			return Calculator::CalculationResult(changed, false);

	}
	return CalculationResult(changed, true);
}

void
WBCalculator::restorePrefix(const ReadLabel *rLab,
			    const std::vector<std::unique_ptr<EventLabel> > &storePrefix,
			    const std::vector<std::pair<Event, Event> > &status)
{
	auto &g = getGraph();
	for (const auto &lab : storePrefix) {
		if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab.get())) {
			trackCoherenceAtLoc(mLab->getAddr());
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(mLab))
				addStoreToLoc(wLab->getAddr(), wLab->getPos(), 0);
		}
	}
}

void WBCalculator::removeAfter(const VectorClock &preds)
{
	auto &g = getGraph();
	VSet<SAddr> keep;

	/* Check which locations should be kept */
	for (auto i = 0u; i < preds.size(); i++) {
		for (auto j = 0u; j <= preds[i]; j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab))
				keep.insert(mLab->getAddr());
		}
	}

	for (auto it = stores_.begin(); it != stores_.end(); /* empty */) {
		it->second.erase(std::remove_if(it->second.begin(), it->second.end(),
						[&](Event &e)
						{ return !preds.contains(e); }),
				 it->second.end());

		/* Should we keep this memory location lying around? */
		if (!keep.count(it->first)) {
			BUG_ON(!it->second.empty());
			it = stores_.erase(it);
		} else {
			++it;
		}
	}
}
