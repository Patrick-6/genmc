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

#include "RC11WBDriver.hpp"

/************************************************************
 ** WB DRIVER
 ***********************************************************/

/* Calculates a minimal hb vector clock based on po for a given label */
View RC11WBDriver::calcBasicHbView(Event e) const
{
	View v(getGraph().getPreviousLabel(e)->getHbView());

	++v[e.thread];
	return v;
}

/* Calculates a minimal (po U rf) vector clock based on po for a given label */
View RC11WBDriver::calcBasicPorfView(Event e) const
{
	View v(getGraph().getPreviousLabel(e)->getPorfView());

	++v[e.thread];
	return v;
}

void RC11WBDriver::calcBasicReadViews(ReadLabel *lab)
{
	const auto &g = getGraph();
	const EventLabel *rfLab = g.getEventLabel(lab->getRf());
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	porf.update(rfLab->getPorfView());
	if (lab->isAtLeastAcquire()) {
		if (auto *wLab = llvm::dyn_cast<WriteLabel>(rfLab))
			hb.update(wLab->getMsgView());
	}
	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
}

void RC11WBDriver::calcBasicWriteViews(WriteLabel *lab)
{
	const auto &g = getGraph();

	/* First, we calculate the hb and (po U rf) views */
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
}

void RC11WBDriver::calcWriteMsgView(WriteLabel *lab)
{
	const auto &g = getGraph();
	View msg;

	/* Should only be called with plain writes */
	BUG_ON(llvm::isa<FaiWriteLabel>(lab) || llvm::isa<CasWriteLabel>(lab));

	if (lab->isAtLeastRelease())
		msg = lab->getHbView();
	else if (lab->getOrdering() == llvm::AtomicOrdering::Monotonic ||
		 lab->getOrdering() == llvm::AtomicOrdering::Acquire)
		msg = g.getHbBefore(g.getLastThreadReleaseAtLoc(lab->getPos(),
								lab->getAddr()));
	lab->setMsgView(std::move(msg));
}

void RC11WBDriver::calcRMWWriteMsgView(WriteLabel *lab)
{
	const auto &g = getGraph();
	View msg;

	/* Should only be called with RMW writes */
	BUG_ON(!llvm::isa<FaiWriteLabel>(lab) && !llvm::isa<CasWriteLabel>(lab));

	const EventLabel *pLab = g.getPreviousLabel(lab);

	BUG_ON(pLab->getOrdering() == llvm::AtomicOrdering::NotAtomic);
	BUG_ON(!llvm::isa<ReadLabel>(pLab));

	const ReadLabel *rLab = static_cast<const ReadLabel *>(pLab);
	if (auto *wLab = llvm::dyn_cast<WriteLabel>(g.getEventLabel(rLab->getRf())))
		msg.update(wLab->getMsgView());

	if (rLab->isAtLeastRelease())
		msg.update(lab->getHbView());
	else
		msg.update(g.getHbBefore(g.getLastThreadReleaseAtLoc(lab->getPos(),
								     lab->getAddr())));

	lab->setMsgView(std::move(msg));
}

void RC11WBDriver::calcFenceRelRfPoBefore(Event last, View &v)
{
	const auto &g = getGraph();
	for (auto i = last.index; i > 0; i--) {
		const EventLabel *lab = g.getEventLabel(Event(last.thread, i));
		if (llvm::isa<FenceLabel>(lab) && lab->isAtLeastAcquire())
			return;
		if (!llvm::isa<ReadLabel>(lab))
			continue;
		auto *rLab = static_cast<const ReadLabel *>(lab);
		if (rLab->getOrdering() == llvm::AtomicOrdering::Monotonic ||
		    rLab->getOrdering() == llvm::AtomicOrdering::Release) {
			const EventLabel *rfLab = g.getEventLabel(rLab->getRf());
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(rfLab))
				v.update(wLab->getMsgView());
		}
	}
}


void RC11WBDriver::calcBasicFenceViews(FenceLabel *lab)
{
	const auto &g = getGraph();
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	if (lab->isAtLeastAcquire())
		calcFenceRelRfPoBefore(lab->getPos().prev(), hb);

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
}

std::unique_ptr<ReadLabel>
RC11WBDriver::createReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			     const llvm::GenericValue *ptr, const llvm::Type *typ,
			     Event rf)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ReadLabel>(g.nextStamp(), ord, pos, ptr, typ, rf);

	calcBasicReadViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<FaiReadLabel>
RC11WBDriver::createFaiReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				const llvm::GenericValue *ptr, const llvm::Type *typ,
				Event rf, llvm::AtomicRMWInst::BinOp op,
				llvm::GenericValue &&opValue)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<FaiReadLabel>(g.nextStamp(), ord, pos, ptr, typ,
						   rf, op, opValue);

	calcBasicReadViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<CasReadLabel>
RC11WBDriver::createCasReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				const llvm::GenericValue *ptr, const llvm::Type *typ,
				Event rf, const llvm::GenericValue &expected,
				const llvm::GenericValue &swap,
				bool isLock)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<CasReadLabel>(g.nextStamp(), ord, pos, ptr, typ,
						   rf, expected, swap, isLock);

	calcBasicReadViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<LibReadLabel>
RC11WBDriver::createLibReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				const llvm::GenericValue *ptr, const llvm::Type *typ,
				Event rf, std::string functionName)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<LibReadLabel>(g.nextStamp(), ord, pos, ptr,
						   typ, rf, functionName);
	calcBasicReadViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<WriteLabel>
RC11WBDriver::createStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
			      const llvm::GenericValue *ptr, const llvm::Type *typ,
			      const llvm::GenericValue &val, int offsetMO,
			      bool isUnlock)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<WriteLabel>(g.nextStamp(), ord, pos, ptr,
						 typ, val, isUnlock);
	calcBasicWriteViews(lab.get());
	calcWriteMsgView(lab.get());
	return std::move(lab);
}

std::unique_ptr<FaiWriteLabel>
RC11WBDriver::createFaiStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				 const llvm::GenericValue *ptr, const llvm::Type *typ,
				 const llvm::GenericValue &val, int offsetMO)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<FaiWriteLabel>(g.nextStamp(), ord, pos,
						    ptr, typ, val);
	calcBasicWriteViews(lab.get());
	calcRMWWriteMsgView(lab.get());
	return std::move(lab);
}

std::unique_ptr<CasWriteLabel>
RC11WBDriver::createCasStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				 const llvm::GenericValue *ptr, const llvm::Type *typ,
				 const llvm::GenericValue &val, int offsetMO,
				 bool isLock)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<CasWriteLabel>(g.nextStamp(), ord, pos, ptr,
						    typ, val, isLock);

	calcBasicWriteViews(lab.get());
	calcRMWWriteMsgView(lab.get());
	return std::move(lab);
}

std::unique_ptr<LibWriteLabel>
RC11WBDriver::createLibStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				 const llvm::GenericValue *ptr, const llvm::Type *typ,
				 llvm::GenericValue &val, int offsetMO,
				 std::string functionName, bool isInit)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<LibWriteLabel>(g.nextStamp(), ord, pos, ptr,
						    typ, val, functionName, isInit);

	calcBasicWriteViews(lab.get());
	calcWriteMsgView(lab.get());
	return std::move(lab);
}

std::unique_ptr<FenceLabel>
RC11WBDriver::createFenceLabel(int tid, int index, llvm::AtomicOrdering ord)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<FenceLabel>(g.nextStamp(), ord, pos);

	calcBasicFenceViews(lab.get());
	return std::move(lab);
}


std::unique_ptr<MallocLabel>
RC11WBDriver::createMallocLabel(int tid, int index, const void *addr,
			       unsigned int size, bool isLocal)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<MallocLabel>(g.nextStamp(),
						  llvm::AtomicOrdering::NotAtomic,
						  pos, addr, size, isLocal);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	return std::move(lab);
}

std::unique_ptr<FreeLabel>
RC11WBDriver::createFreeLabel(int tid, int index, const void *addr,
			     unsigned int size)
{
	auto &g = getGraph();
	Event pos(tid, index);
	std::unique_ptr<FreeLabel> lab(
		new FreeLabel(g.nextStamp(), llvm::AtomicOrdering::NotAtomic,
			      pos, addr, size));

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());


	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	return std::move(lab);
}

std::unique_ptr<ThreadCreateLabel>
RC11WBDriver::createTCreateLabel(int tid, int index, int cid)
{
	const auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadCreateLabel>(getGraph().nextStamp(),
							llvm::AtomicOrdering::Release, pos, cid);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));

	return std::move(lab);
}

std::unique_ptr<ThreadJoinLabel>
RC11WBDriver::createTJoinLabel(int tid, int index, int cid)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadJoinLabel>(g.nextStamp(),
						      llvm::AtomicOrdering::Acquire,
						      pos, cid);

	/* Thread joins have acquire semantics -- but we have to wait
	 * for the other thread to finish first, so we do not fully
	 * update the view yet */
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	return std::move(lab);
}

std::unique_ptr<ThreadStartLabel>
RC11WBDriver::createStartLabel(int tid, int index, Event tc)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadStartLabel>(g.nextStamp(),
						       llvm::AtomicOrdering::Acquire,
						       pos, tc);

	/* Thread start has Acquire semantics */
	View hb(g.getHbBefore(tc));
	View porf(g.getPorfBefore(tc));

	hb[tid] = pos.index;
	porf[tid] = pos.index;

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	return std::move(lab);
}

std::unique_ptr<ThreadFinishLabel>
RC11WBDriver::createFinishLabel(int tid, int index)
{
	const auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadFinishLabel>(getGraph().nextStamp(),
							llvm::AtomicOrdering::Release,
							pos);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	return std::move(lab);
}

/*
 * Checks which of the stores are (rf?;hb)-before some event e, given the
 * hb-before view of e
 */
View RC11WBDriver::getRfOptHbBeforeStores(const std::vector<Event> &stores,
					  const View &hbBefore)
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

void RC11WBDriver::expandMaximalAndMarkOverwritten(const std::vector<Event> &stores,
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
						storeView[w.thread]++;
						break;
					}
				}
			}
		}
	}
	return;
}

std::vector<Event> RC11WBDriver::getStoresToLoc(const llvm::GenericValue *addr)
{
	const auto &g = getGraph();
	auto &thr = getEE()->getCurThr();
	auto &allStores = g.getModOrderAtLoc(addr);
	auto &hbBefore = g.getHbBefore(g.getLastThreadEvent(thr.id));
	std::vector<Event> result;

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
		}
	}
	if (count <= 1)
		return result;

	auto wb = g.calcWb(addr);
	auto &stores = wb.getElems();

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

	return result;
}

std::pair<int, int> RC11WBDriver::getPossibleMOPlaces(const llvm::GenericValue *addr, bool isRMW)
{
	const auto &g = getGraph();
	auto locMOSize = (int) g.getModOrderAtLoc(addr).size();
	return std::make_pair(locMOSize, locMOSize);
}

std::vector<Event> RC11WBDriver::getRevisitLoads(const WriteLabel *sLab)
{
	const auto &g = getGraph();
	auto ls = g.getRevisitable(sLab);

	/* Optimization:
	 * Since sLab is a porf-maximal store, unless it is an RMW, it is
	 * wb-maximal (and so, all revisitable loads can read from it).
	 */
	if (!llvm::isa<FaiWriteLabel>(sLab) && !llvm::isa<CasWriteLabel>(sLab))
		return ls;

	/* Optimization:
	 * If sLab is maximal in WB, then all revisitable loads can read
	 * from it.
	 */
	if (ls.size() > 1) {
		auto wb = g.calcWb(sLab->getAddr());
		auto i = wb.getIndex(sLab->getPos());
		bool allowed = true;
		for (auto j = 0u; j < wb.size(); j++)
			if (wb(i,j)) {
				allowed = false;
				break;
			}
		if (allowed)
			return ls;
	}

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
		auto v = g.getViewFromStamp(g.getEventLabel(l)->getStamp());
		v.update(g.getPorfBefore(sLab->getPos()));

		auto wb = g.calcWbRestricted(sLab->getAddr(), v);
		auto &stores = wb.getElems();
		auto i = wb.getIndex(sLab->getPos());

		auto &hbBefore = g.getHbBefore(l.prev());
		bool allowed = true;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wb(i, j) && g.isWriteRfBefore(hbBefore, stores[j])) {
				allowed = false;
				break;
			}
		}
		if (allowed)
			result.push_back(l);
	}
	return result;
}

std::pair<std::vector<std::unique_ptr<EventLabel> >,
	  std::vector<std::pair<Event, Event> > >
RC11WBDriver::getPrefixToSaveNotBefore(const WriteLabel *wLab, const ReadLabel *rLab)
{
	const auto &g = getGraph();
	auto writePrefix = g.getPrefixLabelsNotBefore(wLab, rLab);
	return std::make_pair(std::move(writePrefix), std::vector<std::pair<Event, Event>>());
}

bool RC11WBDriver::checkPscAcyclicity(CheckPSCType t)
{
	switch (t) {
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

bool RC11WBDriver::isExecutionValid()
{
	return checkPscAcyclicity(CheckPSCType::full);
}
