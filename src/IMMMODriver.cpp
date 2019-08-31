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

void IMMMODriver::restrictGraph(unsigned int stamp)
{
	const auto &g = getGraph();

	/* First, free memory allocated by events that will no longer
	 * be in the graph */
	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = 1u; j < g.getThreadSize(i); j++) {
			const EventLabel *lab = g.getEventLabel(Event(i, j));
			if (lab->getStamp() <= stamp)
				continue;
			if (auto *mLab = llvm::dyn_cast<MallocLabel>(lab))
				getEE()->deallocateAddr(mLab->getAllocAddr(),
							mLab->getAllocSize(),
							mLab->isLocal());
		}
	}

	/* Then, restrict the graph */
	getGraph().cutToStamp(stamp);
	return;
}

int IMMMODriver::splitLocMOBefore(const llvm::GenericValue *addr, const View &before)
{
	const auto &g = getGraph();
	auto &locMO = g.getModOrderAtLoc(addr);

	for (auto rit = locMO.rbegin(); rit != locMO.rend(); ++rit) {
		if (before.empty() || !g.isWriteRfBefore(before, *rit))
			continue;
		return std::distance(rit, locMO.rend());
	}
	return 0;
}

int IMMMODriver::splitLocMOAfter(const llvm::GenericValue *addr, const Event e)
{
	const auto &g = getGraph();
	auto &locMO = g.getModOrderAtLoc(addr);

	for (auto it = locMO.begin(); it != locMO.end(); ++it) {
		if (g.isHbOptRfBefore(e, *it))
			return std::distance(locMO.begin(), it);
	}
	return locMO.size();
}

int IMMMODriver::splitLocMOAfterHb(const llvm::GenericValue *addr, const Event e)
{
	const auto &g = getGraph();
	auto &locMO = g.getModOrderAtLoc(addr);

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

/* Calculates a minimal hb vector clock based on po for a given label */
View IMMMODriver::calcBasicHbView(Event e) const
{
	View v(getGraph().getPreviousLabel(e)->getHbView());

	++v[e.thread];
	return v;
}

/* Calculates a minimal (po U rf) vector clock based on po for a given label */
View IMMMODriver::calcBasicPorfView(Event e) const
{
	View v(getGraph().getPreviousLabel(e)->getPorfView());

	++v[e.thread];
	return v;
}

DepView IMMMODriver::calcPPoView(Event e) /* not const */
{
	auto &g = getGraph();
	auto *EE = getEE();
	DepView v;

	/* Update ppo based on dependencies (addr, data, ctrl, addr;po, cas) */
	auto *addr = EE->getCurrentAddrDeps();
	if (addr) {
		for (auto &adep : *addr)
			v.update(g.getPPoRfBefore(adep));
	}
	auto *data = EE->getCurrentDataDeps();
	if (data) {
		for (auto &ddep : *data)
			v.update(g.getPPoRfBefore(ddep));
	}
	auto *ctrl = EE->getCurrentCtrlDeps();
	if (ctrl) {
		for (auto &cdep : *ctrl)
			v.update(g.getPPoRfBefore(cdep));
	}
	auto *addrPo = EE->getCurrentAddrPoDeps();
	if (addrPo) {
		for (auto &apdep : *addrPo)
			v.update(g.getPPoRfBefore(apdep));
	}
	auto *cas = EE->getCurrentCasDeps();
	if (cas) {
		for (auto &csdep : *cas)
			v.update(g.getPPoRfBefore(csdep));
	}

	/* This event does not depend on anything else */
	int oldIdx = v[e.thread];
	v[e.thread] = e.index;
	for (auto i = oldIdx + 1; i < e.index; i++)
		v.addHole(Event(e.thread, i));

	/* Update based on the view of the last acquire of the thread */
	std::vector<Event> acqs = g.getThreadAcquiresAndFences(e);
	for (auto &ev : acqs)
		v.update(g.getPPoRfBefore(ev));
	return v;
}

void IMMMODriver::calcBasicReadViews(ReadLabel *lab)
{
	const auto &g = getGraph();
	const EventLabel *rfLab = g.getEventLabel(lab->getRf());
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView ppo = calcPPoView(lab->getPos());
	DepView pporf(ppo);

	porf.update(rfLab->getPorfView());
	pporf.update(rfLab->getPPoRfView());
	if (rfLab->getThread() != lab->getThread()) {
		for (auto i = 0u; i < lab->getIndex(); i++) {
			const EventLabel *eLab = g.getEventLabel(Event(lab->getThread(), i));
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(eLab)) {
				if (wLab->getAddr() == lab->getAddr())
					pporf.update(wLab->getPPoRfView());
			}
		}
	}
	if (lab->isAtLeastAcquire()) {
		if (auto *wLab = llvm::dyn_cast<WriteLabel>(rfLab))
			hb.update(wLab->getMsgView());
	}
	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoView(std::move(ppo));
	lab->setPPoRfView(std::move(pporf));
}

void IMMMODriver::calcBasicWriteViews(WriteLabel *lab)
{
	const auto &g = getGraph();

	/* First, we calculate the hb and (po U rf) views */
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));

	/* Then, we calculate the (ppo U rf) view */
	DepView pporf = calcPPoView(lab->getPos());

	if (llvm::isa<CasWriteLabel>(lab) || llvm::isa<FaiWriteLabel>(lab))
		pporf.update(g.getPPoRfBefore(g.getPreviousLabel(lab)->getPos()));

	if (lab->isAtLeastRelease()) {
		pporf.removeAllHoles(lab->getThread());
		Event rel = g.getLastThreadRelease(lab->getPos());
		if (llvm::isa<FaiWriteLabel>(g.getEventLabel(rel)) ||
		    llvm::isa<CasWriteLabel>(g.getEventLabel(rel)))
			--rel.index;
		for (auto i = rel.index; i < lab->getIndex(); i++) {
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(
				    g.getEventLabel(Event(lab->getThread(), i)))) {
				pporf.update(rLab->getPPoRfView());
			}
		}
	}
	pporf.update(g.getPPoRfBefore(g.getLastThreadReleaseAtLoc(lab->getPos(),
								  lab->getAddr())));
	lab->setPPoRfView(std::move(pporf));
}

void IMMMODriver::calcWriteMsgView(WriteLabel *lab)
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

void IMMMODriver::calcRMWWriteMsgView(WriteLabel *lab)
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

void IMMMODriver::calcFenceRelRfPoBefore(Event last, View &v)
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


void IMMMODriver::calcBasicFenceViews(FenceLabel *lab)
{
	const auto &g = getGraph();
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcPPoView(lab->getPos());

	if (lab->isAtLeastAcquire())
		calcFenceRelRfPoBefore(lab->getPos().prev(), hb);
	if (lab->isAtLeastRelease()) {
		pporf.removeAllHoles(lab->getThread());
		Event rel = g.getLastThreadRelease(lab->getPos());
		for (auto i = rel.index; i < lab->getIndex(); i++) {
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(
				    g.getEventLabel(Event(lab->getThread(), i)))) {
				pporf.update(rLab->getPPoRfView());
			}
		}
	}

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoRfView(std::move(pporf));
}

std::unique_ptr<ReadLabel>
IMMMODriver::createReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createFaiReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createCasReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createLibReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createFaiStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createCasStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createLibStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMMODriver::createFenceLabel(int tid, int index, llvm::AtomicOrdering ord)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<FenceLabel>(g.nextStamp(), ord, pos);

	calcBasicFenceViews(lab.get());
	return std::move(lab);
}


std::unique_ptr<MallocLabel>
IMMMODriver::createMallocLabel(int tid, int index, const void *addr,
			       unsigned int size, bool isLocal)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<MallocLabel>(g.nextStamp(),
						  llvm::AtomicOrdering::NotAtomic,
						  pos, addr, size, isLocal);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcPPoView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<FreeLabel>
IMMMODriver::createFreeLabel(int tid, int index, const void *addr,
			     unsigned int size)
{
	auto &g = getGraph();
	Event pos(tid, index);
	std::unique_ptr<FreeLabel> lab(
		new FreeLabel(g.nextStamp(), llvm::AtomicOrdering::NotAtomic,
			      pos, addr, size));

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	WARN("Calculate pporf views!\n");
	BUG();

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	return std::move(lab);
}

std::unique_ptr<ThreadCreateLabel>
IMMMODriver::createTCreateLabel(int tid, int index, int cid)
{
	const auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadCreateLabel>(getGraph().nextStamp(),
							llvm::AtomicOrdering::Release, pos, cid);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcPPoView(lab->getPos());

	pporf.removeAllHoles(lab->getThread());
	Event rel = g.getLastThreadRelease(lab->getPos());
	for (auto i = rel.index; i < lab->getIndex(); i++) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(
			    g.getEventLabel(Event(lab->getThread(), i)))) {
			pporf.update(rLab->getPPoRfView());
		}
	}

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadJoinLabel>
IMMMODriver::createTJoinLabel(int tid, int index, int cid)
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
	DepView pporf = calcPPoView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoView(std::move(pporf));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadStartLabel>
IMMMODriver::createStartLabel(int tid, int index, Event tc)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadStartLabel>(g.nextStamp(),
						       llvm::AtomicOrdering::Acquire,
						       pos, tc);

	/* Thread start has Acquire semantics */
	View hb(g.getHbBefore(tc));
	View porf(g.getPorfBefore(tc));
	DepView pporf(g.getPPoRfBefore(tc));

	hb[tid] = pos.index;
	porf[tid] = pos.index;
	pporf[tid] = pos.index;

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadFinishLabel>
IMMMODriver::createFinishLabel(int tid, int index)
{
	const auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadFinishLabel>(getGraph().nextStamp(),
							llvm::AtomicOrdering::Release,
							pos);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcPPoView(lab->getPos());

	pporf.removeAllHoles(lab->getThread());
	Event rel = g.getLastThreadRelease(lab->getPos());
	for (auto i = rel.index; i < lab->getIndex(); i++) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(
			    g.getEventLabel(Event(lab->getThread(), i)))) {
			pporf.update(rLab->getPPoRfView());
		}
	}

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::vector<Event> IMMMODriver::getStoresToLoc(const llvm::GenericValue *addr)
{
	std::vector<Event> stores;

	const auto &g = getGraph();
	auto &thr = getEE()->getCurThr();
	auto &locMO = g.getModOrderAtLoc(addr);
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
	const auto &g = getGraph();
	auto &thr = getEE()->getCurThr();
	Event curr = Event(thr.id, thr.globalInstructions - 1);

	if (isRMW) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(g.getEventLabel(curr))) {
			auto offset = g.getModOrder().getStoreOffset(addr, rLab->getRf()) + 1;
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
	const auto &g = getGraph();
	auto ls = g.getRevisitable(sLab);
	auto &locMO = g.getModOrderAtLoc(sLab->getAddr());

	// /* If this store is mo-maximal then we are done */
	// if (locMO.back() == sLab->getPos())
	// 	return ls;

	/* Do not revisit hb-before loads */
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ return g.getHbBefore(sLab->getPos()).contains(e); }),
		 ls.end());

	/* Otherwise, we have to exclude (mo;rf?;hb?;sb)-after reads */
	auto moOptRfs = g.getMoOptRfAfter(sLab);
	ls.erase(std::remove_if(ls.begin(), ls.end(), [&](Event e)
				{ const View &before = g.getHbPoBefore(e);
				  return std::any_of(moOptRfs.begin(), moOptRfs.end(),
					 [&](Event ev)
					 { return before.contains(ev); });
				}), ls.end());

	/* ...and we also have to exclude (mo^-1; rf?; (hb^-1)?; sb^-1)-after reads in
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
	const auto &g = getGraph();
	auto prefix = g.getPrefixLabelsNotBefore(sLab, rLab);
	auto moPlacings = g.getMOPredsInBefore(prefix, g.getDepViewFromStamp(rLab->getStamp()));

	BUG_ON(prefix.empty());
	return std::make_pair(std::move(prefix), std::move(moPlacings));
}

void IMMMODriver::changeRf(Event read, Event store)
{
	auto &g = getGraph();

	/* Change the reads-from relation in the graph */
	g.changeRf(read, store);

	/* And update the views of the load */
	EventLabel *lab = g.getEventLabel(read);
	ReadLabel *rLab = static_cast<ReadLabel *>(lab);
	EventLabel *rfLab = g.getEventLabel(store);
	View hb = calcBasicHbView(rLab->getPos());
	View porf = calcBasicPorfView(rLab->getPos());
	DepView pporf(rLab->getPPoView());

	porf.update(rfLab->getPorfView());
	pporf.update(rfLab->getPPoRfView());
	if (rfLab->getThread() != rLab->getThread()) {
		for (auto i = 0u; i < rLab->getIndex(); i++) {
			const EventLabel *eLab = g.getEventLabel(Event(rLab->getThread(), i));
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(eLab)) {
				if (wLab->getAddr() == rLab->getAddr())
					pporf.update(wLab->getPPoRfView());
			}
		}
	}

	if (rLab->isAtLeastAcquire()) {
		if (auto *wLab = llvm::dyn_cast<WriteLabel>(rfLab))
			hb.update(wLab->getMsgView());
	}
	rLab->setHbView(std::move(hb));
	rLab->setPorfView(std::move(porf));
	rLab->setPPoRfView(std::move(pporf));
}

void IMMMODriver::resetJoin(Event join)
{
	auto &g = getGraph();

	g.resetJoin(join);

	EventLabel *jLab = g.getEventLabel(join);
	View hb = calcBasicHbView(jLab->getPos());
	View porf = calcBasicPorfView(jLab->getPos());
	DepView pporf(jLab->getPPoView());

	jLab->setHbView(std::move(hb));
	jLab->setPorfView(std::move(porf));
	jLab->setPPoRfView(std::move(pporf));
	return;
}

bool IMMMODriver::updateJoin(Event join, Event childLast)
{
	auto &g = getGraph();

	if (!g.updateJoin(join, childLast))
		return false;

	EventLabel *jLab = g.getEventLabel(join);
	EventLabel *fLab = g.getEventLabel(childLast);

	jLab->updateHbView(fLab->getHbView());
	jLab->updatePorfView(fLab->getPorfView());
	jLab->updatePPoRfView(fLab->getPPoRfView());
	return true;
}

std::vector<Event> IMMMODriver::collectAllEvents()
{
	const auto &g = getGraph();
	std::vector<Event> result;

	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = 1u; j < g.getThreadSize(i); j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (llvm::isa<MemAccessLabel>(lab) ||
			    llvm::isa<FenceLabel>(lab))
				result.push_back(lab->getPos());
		}
	}
	return result;
}

/* TODO: Add function in VC interface that transforms VC to Matrix? */
void IMMMODriver::fillMatrixFromView(const Event e, const DepView &v,
				     Matrix2D<Event> &matrix)
{
	const auto &g = getGraph();
 	auto eI = matrix.getIndex(e);

	for (auto i = 0u; i < v.size(); i++) {
                if ((int) i == e.thread)
			continue;
		for (auto j = v[i]; j > 0; j--) {
			auto curr = Event(i, j);
			if (!v.contains(curr))
				continue;

			auto *lab = g.getEventLabel(curr);
                        if (!llvm::isa<MemAccessLabel>(lab) &&
			    !llvm::isa<FenceLabel>(lab))
                                continue;

			matrix(lab->getPos(), eI) = true;
                        break;
                }
 	}
}

Matrix2D<Event> IMMMODriver::getARMatrix()
{
	Matrix2D<Event> ar(collectAllEvents());
	const std::vector<Event> &es = ar.getElems();
        auto len = ar.size();
	const auto &g = getGraph();

        /* First, add ppo edges */
        for (auto i = 0u; i < len; i++) {
                for (auto j = 0u; j < len; j++) {
			if (es[i].thread == es[j].thread &&
			    es[i].index < es[j].index &&
			    g.getPPoRfBefore(es[j]).contains(es[i]))
				ar(i, j) = true;
                }
        }

        /* Then, add the remaining ar edges */
        for (const auto &e : es) {
                auto *lab = g.getEventLabel(e);
                BUG_ON(!llvm::isa<MemAccessLabel>(lab) && !llvm::isa<FenceLabel>(lab));
                fillMatrixFromView(e, g.getPPoRfBefore(e), ar);
        }
	ar.transClosure();
	return ar;
}

bool IMMMODriver::checkPscAcyclicity(CheckPSCType t)
{
	switch (t) {
	case CheckPSCType::nocheck:
		return true;
	case CheckPSCType::weak:
	case CheckPSCType::wb:
		WARN_ONCE("check-mo-psc", "WARNING: The full PSC condition is going "
			  "to be checked for the MO-tracking exploration...\n");
	case CheckPSCType::full: {
		return getGraph().isPscAcyclicMO();
		// auto &g = getGraph();
		// Matrix2D<Event> ar = getARMatrix();
		// auto scs = g.getSCs();
		// auto &fcs = scs.second;
		// Matrix2D<Event> psc =  g.calcPscMO();

		// for (auto &f1 : fcs) {
		// 	for (auto &f2 : fcs)
		// 		if (psc(f1, f2))
		// 			ar(f1, f2) = true;
		// }
		// ar.transClosure();
		// return !ar.isReflexive();
	}
	default:
		WARN("Unimplemented model!\n");
		BUG();
	}
}

bool IMMMODriver::isExecutionValid()
{
	return checkPscAcyclicity(CheckPSCType::full);
}
