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

void IMMWBDriver::restrictGraph(unsigned int stamp)
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

/* Calculates a minimal hb vector clock based on po for a given label */
View IMMWBDriver::calcBasicHbView(Event e) const
{
	View v(getGraph().getPreviousLabel(e)->getHbView());

	++v[e.thread];
	return v;
}

/* Calculates a minimal (po U rf) vector clock based on po for a given label */
View IMMWBDriver::calcBasicPorfView(Event e) const
{
	View v(getGraph().getPreviousLabel(e)->getPorfView());

	++v[e.thread];
	return v;
}

DepView IMMWBDriver::calcBasicPPoRfView(Event e) /* not const */
{
	auto &g = getGraph();
	DepView v;

	/* Update ppo based on dependencies (data, addr, ctrl, addr;po) */
	for (auto &adep : g.addrDeps[e])
		v.update(g.getPPoRfBefore(adep));
	for (auto &ddep : g.dataDeps[e])
		v.update(g.getPPoRfBefore(ddep));
	for (auto &cdep : g.ctrlDeps[e])
		v.update(g.getPPoRfBefore(cdep));
	for (auto &apdep : g.addrPoDeps[e])
		v.update(g.getPPoRfBefore(apdep));
	for (auto &csdep : g.casDeps[e])
		v.update(g.getPPoRfBefore(csdep));

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

void IMMWBDriver::calcBasicReadViews(ReadLabel *lab)
{
	const auto &g = getGraph();
	const EventLabel *rfLab = g.getEventLabel(lab->getRf());
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcBasicPPoRfView(lab->getPos());

	porf.update(rfLab->getPorfView());
	pporf.update(rfLab->getPPoRfView());
	if (rfLab->getThread() != lab->getThread()) {
		for (auto i = 0u; i < lab->getIndex(); i++) {
			const EventLabel *eLab = g.getEventLabel(Event(lab->getThread(), i));
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(eLab)) {
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
	lab->setPPoRfView(std::move(pporf));
}

void IMMWBDriver::calcBasicWriteViews(WriteLabel *lab)
{
	const auto &g = getGraph();

	/* First, we calculate the hb and (po U rf) views */
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));

	/* Then, we calculate the (ppo U rf) view */
	DepView pporf = calcBasicPPoRfView(lab->getPos());

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

void IMMWBDriver::calcWriteMsgView(WriteLabel *lab)
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

void IMMWBDriver::calcRMWWriteMsgView(WriteLabel *lab)
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

void IMMWBDriver::calcFenceRelRfPoBefore(Event last, View &v)
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


void IMMWBDriver::calcBasicFenceViews(FenceLabel *lab)
{
	const auto &g = getGraph();
	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcBasicPPoRfView(lab->getPos());

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
IMMWBDriver::createReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createFaiReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createCasReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createLibReadLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createFaiStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createCasStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createLibStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
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
IMMWBDriver::createFenceLabel(int tid, int index, llvm::AtomicOrdering ord)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<FenceLabel>(g.nextStamp(), ord, pos);

	calcBasicFenceViews(lab.get());
	return std::move(lab);
}


std::unique_ptr<MallocLabel>
IMMWBDriver::createMallocLabel(int tid, int index, const void *addr,
			       unsigned int size, bool isLocal)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<MallocLabel>(g.nextStamp(),
						  llvm::AtomicOrdering::NotAtomic,
						  pos, addr, size, isLocal);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcBasicPPoRfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<FreeLabel>
IMMWBDriver::createFreeLabel(int tid, int index, const void *addr,
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
IMMWBDriver::createTCreateLabel(int tid, int index, int cid)
{
	const auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadCreateLabel>(getGraph().nextStamp(),
							llvm::AtomicOrdering::Release, pos, cid);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcBasicPPoRfView(lab->getPos());

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
IMMWBDriver::createTJoinLabel(int tid, int index, int cid)
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
	DepView pporf = calcBasicPPoRfView(lab->getPos());

	lab->setHbView(std::move(hb));
	lab->setPorfView(std::move(porf));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadStartLabel>
IMMWBDriver::createStartLabel(int tid, int index, Event tc)
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
IMMWBDriver::createFinishLabel(int tid, int index)
{
	const auto &g = getGraph();
	Event pos(tid, index);
	auto lab = llvm::make_unique<ThreadFinishLabel>(getGraph().nextStamp(),
							llvm::AtomicOrdering::Release,
							pos);

	View hb = calcBasicHbView(lab->getPos());
	View porf = calcBasicPorfView(lab->getPos());
	DepView pporf = calcBasicPPoRfView(lab->getPos());

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

std::vector<Event> IMMWBDriver::getStoresToLoc(const llvm::GenericValue *addr)
{
	const auto &g = getGraph();
	auto &thr = getEE()->getCurThr();
	// auto &hbBefore = g.getHbBefore(g.getLastThreadEvent(thr.id));
	auto last = Event(thr.id, thr.globalInstructions - 1);
	auto &hbBefore = g.getHbBefore(last);
	auto wb = g.calcWb(addr);
	auto &stores = wb.getElems();
	std::vector<Event> result;
	// llvm::dbgs() << "Last is " << last << "\n";
	/* Find the stores from which we can read-from */
	for (auto i = 0u; i < stores.size(); i++) {
		auto allowed = true;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wb(i, j) && g.isWriteRfBefore(hbBefore, stores[j])) {
				// llvm::dbgs() << "disallowed1 " << stores[i] << "\n";
				allowed = false;
				break;
			}
			if (wb(j, i) && g.isHbOptRfBefore(last.next(), stores[j])) {
				allowed = false;
				// llvm::dbgs() << "disallowed2 " << stores[i] << "\n";
				break;
			}
			// if (last == Event(2,14) && stores[i] == Event(1,7)) {
			// 	llvm::dbgs() << "hb view of " <<  stores[j]
			// 		     << " is " << g.getHbBefore(stores[j]) << "\n";
			// }
		}
		/* We cannot read from hb-after stores... */
		if (g.getHbBefore(stores[i]).contains(last.next())) {
			// llvm::dbgs() << "disallowed 3 " << stores[i] << "\n";
			allowed = false;
		}
		/* Also check for violations against the initializer */
		for (auto i = 0u; i < g.getNumThreads(); i++) {
			for (auto j = 1u; j < g.getThreadSize(i); j++) {
				auto *lab = g.getEventLabel(Event(i, j));
				if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
					if (rLab->getRf().isInitializer() &&
					    rLab->getAddr() == addr &&
					    g.getHbBefore(rLab->getPos()).contains(last.next()))
						allowed = false;
				// llvm::dbgs() << "disallowed4 " << stores[i] << "\n";
			}
		}
		if (allowed) {
			// if (last == Event(2,14) && stores[i] == Event(1,7)) {
			//     llvm::dbgs() << "allowed " << stores[i] << "\n";
			//     llvm::dbgs() << "hb view of store " << g.getHbBefore(stores[i]) << "\n";
			//     llvm::dbgs() << "graph is " << g << "\n";
			// }
			result.push_back(stores[i]);
		}
	}

	/* Also check the initializer event */
	bool allowed = true;
	for (auto j = 0u; j < stores.size(); j++)
		if (g.isWriteRfBefore(hbBefore, stores[j]))
			allowed = false;

	if (allowed && !getEE()->isStackAlloca(addr))
		result.insert(result.begin(), Event::getInitializer());
	return result;
}

std::pair<int, int> IMMWBDriver::getPossibleMOPlaces(const llvm::GenericValue *addr, bool isRMW)
{
	const auto &g = getGraph();
	auto locMOSize = (int) g.getModOrderAtLoc(addr).size();
	return std::make_pair(locMOSize, locMOSize);
}

std::vector<Event> IMMWBDriver::getRevisitLoads(const WriteLabel *sLab)
{
	const auto &g = getGraph();
	auto ls = g.getRevisitable(sLab);

	/* Optimization:
	 * Since sLab is a porf-maximal store, unless it is an RMW, it is
	 * wb-maximal (and so, all revisitable loads can read from it).
	 */
	// if (!llvm::isa<FaiWriteLabel>(sLab) && !llvm::isa<CasWriteLabel>(sLab))
	// 	return ls;

	/* Optimization:
	 * If sLab is maximal in WB, then all revisitable loads can read
	 * from it.
	 */
	// if (ls.size() > 1) {
	// 	auto wb = g.calcWb(sLab->getAddr());
	// 	auto i = wb.getIndex(sLab->getPos());
	// 	bool allowed = true;
	// 	for (auto j = 0u; j < wb.size(); j++)
	// 		if (wb(i,j)) {
	// 			allowed = false;
	// 			break;
	// 		}
	// 	if (allowed)
	// 		return ls;
	// }

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
	// llvm::dbgs() << "In graph " << g << "\n" << *sLab << " tries to revisit\n";
	for (auto &l : ls) {
		// llvm::dbgs() << l << "\n";
		auto v = g.getDepViewFromStamp(g.getEventLabel(l)->getStamp());
		v.update(g.getPPoRfBefore(sLab->getPos()));

		auto wb = g.calcWbRestricted(sLab->getAddr(), v);
		auto &stores = wb.getElems();
		auto i = wb.getIndex(sLab->getPos());

		auto &hbBefore = g.getHbBefore(g.getPreviousNonEmptyLabel(l)->getPos());
		bool allowed = true;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wb(i, j) && g.isWriteRfBefore(hbBefore, stores[j])) {
				allowed = false;
				break;
			}
			if (wb(j, i) && g.isHbOptRfBeforeInView(l, stores[j], v)) {
				// llvm::dbgs() << "no can do because of " << stores[j] << "\n";
				allowed = false;
				break;
			}
		}
		/* Do not revisit hb-before loads... */
		if (g.getHbBefore(stores[i]).contains(l))
			allowed = false;
		/* Also check for violations against the initializer */
		for (auto i = 0u; i < g.getNumThreads(); i++) {
			for (auto j = 1u; j < g.getThreadSize(i); j++) {
				auto *lab = g.getEventLabel(Event(i, j));
				if (lab->getPos() == l)
					continue;
				if (!v.contains(lab->getPos()))
					continue;
				if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
					if (rLab->getRf().isInitializer() &&
					    rLab->getAddr() == sLab->getAddr() &&
					    g.getHbBefore(rLab->getPos()).contains(l))
						allowed = false;
			}
		}
		if (allowed)
			result.push_back(l);
	} // llvm::dbgs() << "\n";
	return result;
}

std::pair<std::vector<std::unique_ptr<EventLabel> >,
	  std::vector<std::pair<Event, Event> > >
IMMWBDriver::getPrefixToSaveNotBefore(const WriteLabel *sLab, const ReadLabel *rLab)
{
	const auto &g = getGraph();
	auto prefix = g.getPrefixLabelsNotBefore(sLab, rLab);

	BUG_ON(prefix.empty());
	return std::make_pair(std::move(prefix), std::vector<std::pair<Event, Event>>());
}

bool IMMWBDriver::checkPscAcyclicity(CheckPSCType t)
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

bool IMMWBDriver::isExecutionValid()
{
	return checkPscAcyclicity(CheckPSCType::full);
}
