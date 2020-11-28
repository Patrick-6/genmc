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

#include "config.h"
#include "LKMMDriver.hpp"
#include "PROPCalculator.hpp"
#include "ARCalculatorLKMM.hpp"
#include "PBCalculator.hpp"
#include "RCULinkCalculator.hpp"
#include "RCUCalculator.hpp"
#include "RCUFenceCalculator.hpp"
#include "XBCalculator.hpp"

static const std::unordered_map<std::string, SmpFenceType> smpFenceTypes = {
	{"mb", SmpFenceType::MB},
	{"rmb", SmpFenceType::RMB},
	{"wmb", SmpFenceType::WMB},
	{"ba", SmpFenceType::MBBA},
	{"aa", SmpFenceType::MBAA},
	{"as", SmpFenceType::MBAS},
	{"aul", SmpFenceType::MBAUL},
};

LKMMDriver::LKMMDriver(std::unique_ptr<Config> conf, std::unique_ptr<llvm::Module> mod,
		       std::vector<Library> &granted, std::vector<Library> &toVerify,
		       clock_t start)
	: GenMCDriver(std::move(conf), std::move(mod), granted, toVerify, start)
{
	auto &g = getGraph();

	/* LKMM adds a prop and an xb calculator to the party */
	g.addCalculator(LLVM_MAKE_UNIQUE<PROPCalculator>(g),
			ExecutionGraph::RelationId::prop, false);
	g.addCalculator(LLVM_MAKE_UNIQUE<ARCalculatorLKMM>(g),
			ExecutionGraph::RelationId::ar_lkmm, false);
	g.addCalculator(LLVM_MAKE_UNIQUE<PBCalculator>(g),
			ExecutionGraph::RelationId::pb, false);
	g.addCalculator(LLVM_MAKE_UNIQUE<RCULinkCalculator>(g),
			ExecutionGraph::RelationId::rcu_link, false);
	g.addCalculator(LLVM_MAKE_UNIQUE<RCUCalculator>(g),
			ExecutionGraph::RelationId::rcu, false);
	g.addCalculator(LLVM_MAKE_UNIQUE<RCUFenceCalculator>(g),
			ExecutionGraph::RelationId::rcu_fence, false);
	g.addCalculator(LLVM_MAKE_UNIQUE<XBCalculator>(g),
			ExecutionGraph::RelationId::xb, false);
	return;
}

/* Calculates a minimal hb vector clock based on po for a given label */
View LKMMDriver::calcBasicHbView(Event e) const
{
	View v(getGraph().getPreviousLabel(e)->getHbView());

	++v[e.thread];
	return v;
}

DepView LKMMDriver::calcFenceView(const MemAccessLabel *lab) const
{
	auto &g = getGraph();
	DepView fence;

	for (auto i = 1; i < lab->getIndex(); i++) {
		auto *eLab = g.getEventLabel(Event(lab->getThread(), i));
		if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(eLab)) {
			if (fLab->isStrong())
				fence.update(fLab->getPPoRfView());
			if (fLab->getType() == SmpFenceType::WMB &&
			    llvm::isa<WriteLabel>(lab))
				fence.update(fLab->getPPoRfView());
		}
		if (llvm::isa<FenceLabel>(eLab) && eLab->isAtLeastRelease())
			fence.update(eLab->getPPoRfView());
	}
	return fence;
}

DepView LKMMDriver::calcPPoView(EventLabel *lab) /* not const */
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
	if (ctrl && llvm::isa<WriteLabel>(lab)) { /* LKMM only keeps ctrl deps to writes */
		for (auto &cdep : *ctrl)
			v.update(g.getPPoRfBefore(cdep));
	}
	auto *addrPo = EE->getCurrentAddrPoDeps();
	if (addrPo) {
		; /* LKMM does not include addr;po in ppo */
	}
	auto *cas = EE->getCurrentCasDeps();
	if (cas) {
		for (auto &csdep : *cas)
			v.update(g.getPPoRfBefore(csdep));
	}

	/* This event does not depend on anything else */
	Event e = lab->getPos();
	int oldIdx = v[e.thread];
	v[e.thread] = e.index;
	for (auto i = oldIdx + 1; i < e.index; i++)
		v.addHole(Event(e.thread, i));

	/* Update based on the views of the acquires of the thread */
	std::vector<Event> acqs = g.getThreadAcquiresAndFences(e);
	for (auto &ev : acqs) {
		auto *eLab = g.getEventLabel(ev);
		/* Make sure that smp_wmb() and smp_rmb() only
		 * apply to stores and loads, respectively */
		if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(eLab)) {
			if (fLab->getType() == SmpFenceType::WMB && !llvm::isa<WriteLabel>(lab))
				continue;
			if (fLab->getType() == SmpFenceType::RMB) {
				if (!llvm::isa<ReadLabel>(lab))
					continue;
				auto *rLab = llvm::dyn_cast<FaiReadLabel>(lab);
				if (rLab && rLab->getFaiType() == FaiReadLabel::NoRet)
					continue;
			}
		}
		v.update(g.getPPoRfBefore(ev));
	}
	return v;
}

void LKMMDriver::updateRelView(DepView &pporf, const EventLabel *lab)
{
	if (!lab->isAtLeastRelease())
		return;

	const auto &g = getGraph();

	pporf.removeAllHoles(lab->getThread());

	Event rel = g.getLastThreadRelease(lab->getPos());
	pporf.update(g.getEventLabel(rel)->getPPoRfView());

	if (llvm::isa<FaiWriteLabel>(g.getEventLabel(rel)) ||
	    llvm::isa<CasWriteLabel>(g.getEventLabel(rel)))
		--rel.index;
	for (auto i = rel.index; i < lab->getIndex(); i++) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(
			    g.getEventLabel(Event(lab->getThread(), i)))) {
			pporf.update(rLab->getPPoRfView());
		}
	}
	return;
}

void LKMMDriver::updateReadViewsFromRf(DepView &pporf, View &hb, ReadLabel *lab)
{
	auto &g = getGraph();
	const EventLabel *rfLab = g.getEventLabel(lab->getRf());

	if (rfLab->getThread() == lab->getThread() || rfLab->getPos().isInitializer()) {
		pporf.update(rfLab->getPPoView()); /* Account for dep; rfi dependencies */
		pporf.addHole(rfLab->getPos());    /* Make sure we don't depend on rfi */
	} else {
		pporf.update(rfLab->getPPoRfView());
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
	return;
}

void LKMMDriver::calcBasicReadViews(ReadLabel *lab)
{
	const auto &g = getGraph();
	View hb = calcBasicHbView(lab->getPos());
	DepView fence = calcFenceView(lab);
	DepView ppo = calcPPoView(lab);
	DepView pporf(ppo);

	updateReadViewsFromRf(pporf, hb, lab);

	lab->setHbView(std::move(hb));
	lab->setFenceView(std::move(fence));
	lab->setPPoView(std::move(ppo));
	lab->setPPoRfView(std::move(pporf));
}

void LKMMDriver::calcBasicWriteViews(WriteLabel *lab)
{
	const auto &g = getGraph();

	/* First, we calculate the hb view */
	View hb = calcBasicHbView(lab->getPos());
	lab->setHbView(std::move(hb));

	/* Then, we calculate the ppo, (ppo U rf), and fence views
	 * The first is important because we have to take dep;rfi
	 * dependencies into account for subsequent reads. */
	DepView ppo = calcPPoView(lab);
	DepView fence = calcFenceView(lab);
	DepView pporf(ppo);

	if (llvm::isa<CasWriteLabel>(lab) || llvm::isa<FaiWriteLabel>(lab)) {
		ppo.update(g.getPreviousLabel(lab)->getPPoRfView());
		pporf.update(g.getPreviousLabel(lab)->getPPoRfView());
	}
	if (lab->isAtLeastRelease()) {
		updateRelView(pporf, lab);
		fence = pporf;
	}
	lab->setFenceView(std::move(fence));
	lab->setPPoView(std::move(ppo));
	lab->setPPoRfView(std::move(pporf));
}

void LKMMDriver::calcWriteMsgView(WriteLabel *lab)
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

void LKMMDriver::calcRMWWriteMsgView(WriteLabel *lab)
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

void LKMMDriver::calcFenceRelRfPoBefore(Event last, View &v)
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

void LKMMDriver::updateRmbFenceView(DepView &pporf, SmpFenceLabelLKMM *fLab)
{
	BUG_ON(fLab->getType() != SmpFenceType::RMB);

	for (auto i = 1; i < fLab->getIndex(); i++) {
		auto *lab = getGraph().getEventLabel(Event(fLab->getThread(), i));
		if (auto *rLab = llvm::dyn_cast<FaiReadLabel>(lab))
			if (rLab->getFaiType() == FaiReadLabel::NoRet)
				continue;
		if (llvm::isa<ReadLabel>(lab))
			pporf.update(lab->getPPoRfView());
	}
}

void LKMMDriver::updateWmbFenceView(DepView &pporf, SmpFenceLabelLKMM *fLab)
{
	BUG_ON(fLab->getType() != SmpFenceType::WMB);

	for (auto i = 1; i < fLab->getIndex(); i++) {
		auto *lab = getGraph().getEventLabel(Event(fLab->getThread(), i));
		if (llvm::isa<WriteLabel>(lab))
			pporf.update(lab->getPPoRfView());
	}
}

void LKMMDriver::updateMbFenceView(DepView &pporf, SmpFenceLabelLKMM *fLab)
{
	BUG_ON(!fLab->isStrong());

	pporf.removeAllHoles(fLab->getThread());
	for (auto i = 1; i < fLab->getIndex(); i++) {
		auto *lab = getGraph().getEventLabel(Event(fLab->getThread(), i));
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
			pporf.update(rLab->getPPoRfView());
	}
}

void LKMMDriver::calcBasicFenceViews(SmpFenceLabelLKMM *lab)
{
	const auto &g = getGraph();
	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab);

	if (lab->isAtLeastAcquire())
		calcFenceRelRfPoBefore(lab->getPos().prev(), hb);
	if (lab->isAtLeastRelease())
		updateRelView(pporf, lab);

	switch (lab->getType()) {
	case SmpFenceType::WMB:
		updateWmbFenceView(pporf, lab);
		break;
	case SmpFenceType::RMB:
		updateRmbFenceView(pporf, lab);
		break;
	case SmpFenceType::MB:
	case SmpFenceType::MBBA:
	case SmpFenceType::MBAA:
	case SmpFenceType::MBAS:
	case SmpFenceType::MBAUL:
		updateMbFenceView(pporf, lab);
		break;
	default:
		BUG();
	}

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
}

std::unique_ptr<ReadLabel>
LKMMDriver::createReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			     const llvm::GenericValue *ptr, const llvm::Type *typ,
			     Event rf)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<ReadLabel>(g.nextStamp(), ord, pos, ptr, typ, rf);

	calcBasicReadViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<FaiReadLabel>
LKMMDriver::createFaiReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			       const llvm::GenericValue *ptr, const llvm::Type *typ,
			       Event rf, llvm::AtomicRMWInst::BinOp op,
			       const llvm::GenericValue &opValue,
			       FaiReadLabel::FaiType ft)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<FaiReadLabel>(g.nextStamp(), ord, pos, ptr, typ,
						  rf, op, opValue, ft);

	calcBasicReadViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<CasReadLabel>
LKMMDriver::createCasReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				const llvm::GenericValue *ptr, const llvm::Type *typ,
				Event rf, const llvm::GenericValue &expected,
				const llvm::GenericValue &swap, CasReadLabel::CasType casType)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<CasReadLabel>(g.nextStamp(), ord, pos, ptr, typ,
						  rf, expected, swap, casType);

	calcBasicReadViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<LibReadLabel>
LKMMDriver::createLibReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			      const llvm::GenericValue *ptr, const llvm::Type *typ,
			      Event rf, std::string functionName)
{
	BUG();
	// auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = LLVM_MAKE_UNIQUE<LibReadLabel>(g.nextStamp(), ord, pos, ptr,
	// 					   typ, rf, functionName);
	// calcBasicReadViews(lab.get());
	// return std::move(lab);
}

std::unique_ptr<DskReadLabel>
LKMMDriver::createDskReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			      const llvm::GenericValue *ptr, const llvm::Type *typ,
			      Event rf)
{
	BUG();
	// auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = llvm::make_unique<DskReadLabel>(g.nextStamp(), ord, pos, ptr,
	// 					   typ, rf);
	// calcBasicReadViews(lab.get());
	// if (getConf()->persevere)
	// 	g.getPersChecker()->calcMemAccessPbView(lab.get());
	// return std::move(lab);
}

std::unique_ptr<WriteLabel>
LKMMDriver::createStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
			    const llvm::GenericValue *ptr, const llvm::Type *typ,
			    const llvm::GenericValue &val, bool isUnlock)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<WriteLabel>(g.nextStamp(), ord, pos, ptr,
						 typ, val, isUnlock);
	calcBasicWriteViews(lab.get());
	calcWriteMsgView(lab.get());
	return std::move(lab);
}

std::unique_ptr<FaiWriteLabel>
LKMMDriver::createFaiStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				 const llvm::GenericValue *ptr, const llvm::Type *typ,
				 const llvm::GenericValue &val)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<FaiWriteLabel>(g.nextStamp(), ord, pos,
						    ptr, typ, val);
	calcBasicWriteViews(lab.get());
	calcRMWWriteMsgView(lab.get());
	return std::move(lab);
}

std::unique_ptr<CasWriteLabel>
LKMMDriver::createCasStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				 const llvm::GenericValue *ptr, const llvm::Type *typ,
				 const llvm::GenericValue &val, CasReadLabel::CasType casType)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<CasWriteLabel>(g.nextStamp(), ord, pos, ptr,
						    typ, val, casType);

	calcBasicWriteViews(lab.get());
	calcRMWWriteMsgView(lab.get());
	return std::move(lab);
}

std::unique_ptr<LibWriteLabel>
LKMMDriver::createLibStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
			       const llvm::GenericValue *ptr, const llvm::Type *typ,
			       llvm::GenericValue &val, std::string functionName,
			       bool isInit)
{
	BUG();
	// auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = LLVM_MAKE_UNIQUE<LibWriteLabel>(g.nextStamp(), ord, pos, ptr,
	// 					    typ, val, functionName, isInit);

	// calcBasicWriteViews(lab.get());
	// calcWriteMsgView(lab.get());
	// return std::move(lab);
}

std::unique_ptr<DskWriteLabel>
LKMMDriver::createDskWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
			       const llvm::GenericValue *ptr, const llvm::Type *typ,
			       const llvm::GenericValue &val, void *mapping)
{
	BUG();
	// auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = llvm::make_unique<DskWriteLabel>(
	// 	g.nextStamp(), ord, pos, ptr, typ, val, mapping);

	// calcBasicWriteViews(lab.get());
	// calcWriteMsgView(lab.get());
	// if (getConf()->persevere)
	// 	g.getPersChecker()->calcMemAccessPbView(lab.get());
	// return std::move(lab);
}

std::unique_ptr<DskMdWriteLabel>
LKMMDriver::createDskMdWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
				 const llvm::GenericValue *ptr, const llvm::Type *typ,
				 const llvm::GenericValue &val, void *mapping,
				 std::pair<void *, void *> ordDataRange)
{
	BUG();
	// auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = llvm::make_unique<DskMdWriteLabel>(
	// 	g.nextStamp(), ord, pos, ptr, typ, val, mapping, ordDataRange);

	// calcBasicWriteViews(lab.get());
	// calcWriteMsgView(lab.get());
	// if (getConf()->persevere)
	// 	g.getPersChecker()->calcMemAccessPbView(lab.get());
	// return std::move(lab);
}

std::unique_ptr<DskDirWriteLabel>
LKMMDriver::createDskDirWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
				  const llvm::GenericValue *ptr, const llvm::Type *typ,
				  const llvm::GenericValue &val, void *mapping)
{
	BUG();
	// auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = llvm::make_unique<DskDirWriteLabel>(
	// 	g.nextStamp(), ord, pos, ptr, typ, val, mapping);

	// calcBasicWriteViews(lab.get());
	// calcWriteMsgView(lab.get());
	// if (getConf()->persevere)
	// 	g.getPersChecker()->calcMemAccessPbView(lab.get());
	// return std::move(lab);
}

std::unique_ptr<DskJnlWriteLabel>
LKMMDriver::createDskJnlWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
				  const llvm::GenericValue *ptr, const llvm::Type *typ,
				  const llvm::GenericValue &val, void *mapping, void *transInode)
{
	BUG();
	// auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = llvm::make_unique<DskJnlWriteLabel>(
	// 	g.nextStamp(), ord, pos, ptr, typ, val, mapping, transInode);

	// calcBasicWriteViews(lab.get());
	// calcWriteMsgView(lab.get());
	// if (getConf()->persevere)
	// 	g.getPersChecker()->calcMemAccessPbView(lab.get());
	// return std::move(lab);
}

std::unique_ptr<FenceLabel>
LKMMDriver::createFenceLabel(int tid, int index, llvm::AtomicOrdering ord)
{
	ERROR("(R)C11-style fences cannot be used with -lkmm!\n");
	return nullptr;
}

std::unique_ptr<SmpFenceLabelLKMM>
LKMMDriver::createSmpFenceLabelLKMM(int tid, int index, const char *lkmmType)
{
	auto &g = getGraph();
	Event pos(tid, index);

	BUG_ON(smpFenceTypes.count(lkmmType) == 0);
	auto typ = smpFenceTypes.at(lkmmType);

	auto ord = (isStrong(typ)) ? llvm::AtomicOrdering::SequentiallyConsistent :
		llvm::AtomicOrdering::Monotonic;
	auto lab = LLVM_MAKE_UNIQUE<SmpFenceLabelLKMM>(
		g.nextStamp(), ord, typ, pos);

	calcBasicFenceViews(lab.get());
	return std::move(lab);
}

std::unique_ptr<MallocLabel>
LKMMDriver::createMallocLabel(int tid, int index, const void *addr,
			      unsigned int size, Storage s, AddressSpace spc)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<MallocLabel>(
		g.nextStamp(), llvm::AtomicOrdering::NotAtomic, pos, addr, size, s, spc);

	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab.get());

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}



std::unique_ptr<DskOpenLabel>
LKMMDriver::createDskOpenLabel(int tid, int index, const char *fileName,
			       const llvm::GenericValue &fd)
{
	BUG();
}

std::unique_ptr<DskFsyncLabel>
LKMMDriver::createDskFsyncLabel(int tid, int index, const void *inode,
				unsigned int size)
{
	BUG();
}

std::unique_ptr<DskSyncLabel>
LKMMDriver::createDskSyncLabel(int tid, int index)
{
	BUG();
}

std::unique_ptr<DskPbarrierLabel>
LKMMDriver::createDskPbarrierLabel(int tid, int index)
{
	BUG();
}

std::unique_ptr<FreeLabel>
LKMMDriver::createFreeLabel(int tid, int index, const void *addr)
{
	auto &g = getGraph();
	Event pos(tid, index);
	std::unique_ptr<FreeLabel> lab(
		new FreeLabel(g.nextStamp(), llvm::AtomicOrdering::NotAtomic, pos, addr));

	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab.get());

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadCreateLabel>
LKMMDriver::createTCreateLabel(int tid, int index, int cid)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<ThreadCreateLabel>(
		g.nextStamp(), llvm::AtomicOrdering::Release, pos, cid);

	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab.get());

	pporf.removeAllHoles(lab->getThread());
	Event rel = g.getLastThreadRelease(lab->getPos());
	for (auto i = rel.index; i < lab->getIndex(); i++) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(
			    g.getEventLabel(Event(lab->getThread(), i)))) {
			pporf.update(rLab->getPPoRfView());
		}
	}

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadJoinLabel>
LKMMDriver::createTJoinLabel(int tid, int index, int cid)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<ThreadJoinLabel>(g.nextStamp(),
						      llvm::AtomicOrdering::Acquire,
						      pos, cid);

	/* Thread joins have acquire semantics -- but we have to wait
	 * for the other thread to finish first, so we do not fully
	 * update the view yet */
	View hb = calcBasicHbView(lab->getPos());
	DepView ppo = calcPPoView(lab.get());
	DepView pporf(ppo);

	lab->setHbView(std::move(hb));
	lab->setPPoView(std::move(ppo));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadStartLabel>
LKMMDriver::createStartLabel(int tid, int index, Event tc, int symm /* = -1 */)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<ThreadStartLabel>(
		g.nextStamp(), llvm::AtomicOrdering::Acquire, pos, tc, symm);

	/* Thread start has Acquire semantics */
	View hb(g.getHbBefore(tc));
	DepView pporf(g.getPPoRfBefore(tc));

	hb[tid] = pos.index;
	pporf[tid] = pos.index;

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<ThreadFinishLabel>
LKMMDriver::createFinishLabel(int tid, int index)
{
	auto &g = getGraph();
	Event pos(tid, index);
	auto lab = LLVM_MAKE_UNIQUE<ThreadFinishLabel>(
		g.nextStamp(), llvm::AtomicOrdering::Release, pos);

	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab.get());

	pporf.removeAllHoles(lab->getThread());
	Event rel = g.getLastThreadRelease(lab->getPos());
	for (auto i = rel.index; i < lab->getIndex(); i++) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(
			    g.getEventLabel(Event(lab->getThread(), i)))) {
			pporf.update(rLab->getPPoRfView());
		}
	}

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<LockLabelLAPOR>
LKMMDriver::createLockLabelLAPOR(int tid, int index, const llvm::GenericValue *addr)
{
	BUG();
	// const auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = LLVM_MAKE_UNIQUE<LockLabelLAPOR>(getGraph().nextStamp(),
	// 					     llvm::AtomicOrdering::Acquire,
	// 					     pos, addr);

	// View hb = calcBasicHbView(lab->getPos());
	// DepView pporf = calcPPoView(lab.get());

	// auto prevUnlock = g.getLastThreadUnlockAtLocLAPOR(lab->getPos().prev(),
	// 						  addr);
	// if (!prevUnlock.isInitializer())
	// 	pporf.update(g.getPPoRfBefore(prevUnlock));

	// lab->setHbView(std::move(hb));
	// lab->setPPoRfView(std::move(pporf));
	// return std::move(lab);
}

std::unique_ptr<UnlockLabelLAPOR>
LKMMDriver::createUnlockLabelLAPOR(int tid, int index, const llvm::GenericValue *addr)
{
	BUG();
	// const auto &g = getGraph();
	// Event pos(tid, index);
	// auto lab = LLVM_MAKE_UNIQUE<UnlockLabelLAPOR>(getGraph().nextStamp(),
	// 					       llvm::AtomicOrdering::Release,
	// 					       pos, addr);

	// View hb = calcBasicHbView(lab->getPos());
	// DepView pporf = calcPPoView(lab.get());

	// updateRelView(pporf, lab.get());

	// lab->setHbView(std::move(hb));
	// lab->setPPoRfView(std::move(pporf));
	// return std::move(lab);
}

std::unique_ptr<RCULockLabelLKMM>
LKMMDriver::createRCULockLabelLKMM(int tid, int index)
{
	auto &g = getGraph();
	Event pos(tid, index);

	auto lab = LLVM_MAKE_UNIQUE<RCULockLabelLKMM>(
		g.nextStamp(), llvm::AtomicOrdering::Acquire, pos);

	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab.get());

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<RCUUnlockLabelLKMM>
LKMMDriver::createRCUUnlockLabelLKMM(int tid, int index)
{
	auto &g = getGraph();
	Event pos(tid, index);

	auto lab = LLVM_MAKE_UNIQUE<RCUUnlockLabelLKMM>(
		g.nextStamp(), llvm::AtomicOrdering::Release, pos);

	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab.get());

	updateRelView(pporf, lab.get());

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

std::unique_ptr<RCUSyncLabelLKMM>
LKMMDriver::createRCUSyncLabelLKMM(int tid, int index)
{
	auto &g = getGraph();
	Event pos(tid, index);

	auto lab = LLVM_MAKE_UNIQUE<RCUSyncLabelLKMM>(
		g.nextStamp(), llvm::AtomicOrdering::SequentiallyConsistent, pos);

	View hb = calcBasicHbView(lab->getPos());
	DepView pporf = calcPPoView(lab.get());

	calcFenceRelRfPoBefore(lab->getPos().prev(), hb);
	updateRelView(pporf, lab.get());

	lab->setHbView(std::move(hb));
	lab->setPPoRfView(std::move(pporf));
	return std::move(lab);
}

bool LKMMDriver::areInPotentialRace(const MemAccessLabel *aLab, const MemAccessLabel *bLab)
{
	/* If there is an HB ordering between the two events, there is no race */
	if (isHbBefore(aLab->getPos(), bLab->getPos()) ||
	    isHbBefore(bLab->getPos(), aLab->getPos()) || aLab == bLab)
		return false;

	/* If both accesses are atomic, there is no race */
	/* Note: one check suffices because a variable is either
	 * atomic or not atomic, but we have not checked the address yet */
	if (!aLab->isNotAtomic() && !bLab->isNotAtomic())
		return false;

	/* If they access a different address, there is no race */
	if (aLab->getAddr() != bLab->getAddr())
		return false;

	/* If LAPOR is disabled, we are done */
	if (!getConf()->LAPOR)
		return true;

	/* Otherwise, we have to make sure that the two accesses do _not_
	 * belong in critical sections of the same lock */
	const auto &g = getGraph();
	auto aLock = g.getLastThreadUnmatchedLockLAPOR(aLab->getPos());
	auto bLock = g.getLastThreadUnmatchedLockLAPOR(bLab->getPos());

	/* If any of the two is _not_ in a CS, it is a race */
	if (aLock.isInitializer() || bLock.isInitializer())
		return true;

	/* If both are in a CS, being in CSs of different locks is a race */
	return llvm::dyn_cast<LockLabelLAPOR>(g.getEventLabel(aLock))->getLockAddr() !=
	       llvm::dyn_cast<LockLabelLAPOR>(g.getEventLabel(bLock))->getLockAddr();
}

std::vector<Event> LKMMDriver::findPotentialRacesForNewLoad(const ReadLabel *rLab)
{
	const auto &g = getGraph();
	const View &before = g.getPreviousNonEmptyLabel(rLab)->getHbView();
	const auto &stores = g.getStoresToLoc(rLab->getAddr());
	std::vector<Event> potential;

	/* If there are not any events hb-before the read, there is nothing to do */
	if (before.empty())
		return potential;

	/* Check for events that race with the current load */
	for (auto &s : stores) {
		if (before.contains(s))
			continue;

		auto *sLab = static_cast<const WriteLabel *>(g.getEventLabel(s));
		if (areInPotentialRace(rLab, sLab))
			potential.push_back(s); /* Potential race */
	}
	return potential;
}

std::vector<Event> LKMMDriver::findPotentialRacesForNewStore(const WriteLabel *wLab)
{
	const auto &g = getGraph();
	auto &before = g.getPreviousNonEmptyLabel(wLab)->getHbView();
	std::vector<Event> potential;

	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = before[i] + 1u; j < g.getThreadSize(i); j++) {
			const EventLabel *oLab = g.getEventLabel(Event(i, j));
			if (!llvm::isa<MemAccessLabel>(oLab))
				continue;

			auto *mLab = static_cast<const MemAccessLabel *>(oLab);
			if (areInPotentialRace(wLab, mLab))
				potential.push_back(mLab->getPos()); /* Potential race */
		}
	}
	return potential;
}

bool LKMMDriver::isNoRetRead(const EventLabel *lab)
{
	auto &g = getGraph();
	auto *rLab = llvm::dyn_cast<FaiReadLabel>(lab);
	return rLab && rLab->getFaiType() == FaiReadLabel::NoRet;
}

bool LKMMDriver::isNoRetRead(Event r)
{
	return isNoRetRead(getGraph().getEventLabel(r));
}

bool LKMMDriver::isAcqPoOrPoRelBefore(Event a, Event b)
{
	if (a.thread != b.thread)
		return false;

	auto &g = getGraph();
	auto *labA = g.getEventLabel(a);
	auto *labB = g.getEventLabel(b);

	return (labA->isAtLeastAcquire() && b.index > a.index) ||
	       (labB->isAtLeastRelease() && a.index < b.index);
}

bool LKMMDriver::isFenceBefore(Event a, Event b)
{
	auto &g = getGraph();
	auto &rcu = g.getGlobalRelation(ExecutionGraph::RelationId::rcu);
	auto &rcuFence = g.getGlobalRelation(ExecutionGraph::RelationId::rcu_fence);
	auto *labA = g.getEventLabel(a);
	auto *labB = g.getEventLabel(b);

	/* If a and b do not belong in the same thread, they have to be connected by an rcu fence */
	if (rcuFence(a, b))
		return true;
	if (a.thread != b.thread)
		return false;

	if (isAcqPoOrPoRelBefore(a, b))
		return true;

	auto mm = std::minmax(a.index, b.index);
	auto minI = mm.first;
	auto maxI = mm.second;
	for (auto i = minI; i < maxI; i++) {
		auto *eLab = g.getEventLabel(Event(a.thread, i));
		if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(eLab)) {
			if (fLab->getType() == SmpFenceType::RMB) {
				if (llvm::isa<ReadLabel>(labA) && llvm::isa<ReadLabel>(labB) &&
				    !isNoRetRead(labA->getPos()) && !isNoRetRead(labB->getPos()))
					return true;
			} else if (fLab->getType() == SmpFenceType::WMB) {
				if (llvm::isa<WriteLabel>(labA) && llvm::isa<WriteLabel>(labB))
					return true;
			} else if (fLab->isStrong()) {
				return true;
			}
		} else if (auto *sLab = llvm::dyn_cast<RCUSyncLabelLKMM>(eLab)) {
			return true;
		}
	}
	return false;
}

std::vector<Event> LKMMDriver::getOverwrites(const MemAccessLabel *lab)
{
	auto &g = getGraph();
	auto &co = g.getPerLocRelation(ExecutionGraph::RelationId::co)[lab->getAddr()];
	auto &stores = co.getElems();

	auto from = lab->getPos();
	if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
		from = rLab->getRf();

	if (from.isInitializer())
		return stores;

	std::vector<Event> owrs;
	std::copy_if(stores.begin(), stores.end(), std::back_inserter(owrs),
		     [&](Event s){ return co(from, s); });
	return owrs;
}

std::vector<Event> LKMMDriver::getMarkedWritePreds(const EventLabel *lab)
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);
	auto &elems = prop.getElems();
	std::vector<Event> preds;

	/* Otherwise, check whether any other marked access is fence-before lab */
	std::copy_if(elems.begin(), elems.end(), std::back_inserter(preds),
		     [&](Event e){ return isFenceBefore(e, lab->getPos()) ||
				     lab->getPPoRfView().contains(e); });
	return preds;
}

std::vector<Event> LKMMDriver::getMarkedWriteSuccs(const EventLabel *lab)
{
	auto &g = getGraph();
	auto &prop = g.getGlobalRelation(ExecutionGraph::RelationId::prop);
	auto &elems = prop.getElems();
	std::vector<Event> succs;

	if (!lab->isNotAtomic())
		succs.push_back(lab->getPos());

	/* Otherwise, check whether lab is fence-before any other marked access */
	std::copy_if(elems.begin(), elems.end(), std::back_inserter(succs),
		     [&](Event e){ return isFenceBefore(lab->getPos(), e); });
	return succs;
}

std::vector<Event> LKMMDriver::getMarkedReadPreds(const EventLabel *lab)
{
	auto &g = getGraph();
	std::vector<Event> preds;

	if (!lab->isNotAtomic())
		preds.push_back(lab->getPos());

	bool rmb = false;
	bool sfence = false;
	bool isLabNoRetRead = isNoRetRead(lab->getPos());
	for (auto i = lab->getIndex() - 1; i > 0; i--) {
		auto *eLab = g.getEventLabel(Event(lab->getThread(), i));
		if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(eLab)) {
			if (fLab->isStrong())
				sfence = true;
			if (fLab->getType() == SmpFenceType::RMB)
				rmb = true;
			continue;
		}
		if (auto *fLab = llvm::dyn_cast<RCUSyncLabelLKMM>(eLab)) {
			sfence = true;
			continue;
		}
		if (PROPCalculator::isNonTrivial(eLab) && !eLab->isNotAtomic() && sfence)
			preds.push_back(eLab->getPos());
		if (eLab->isAtLeastAcquire())
			preds.push_back(eLab->getPos());
		if (llvm::isa<ReadLabel>(eLab) && !eLab->isNotAtomic() && rmb && !isLabNoRetRead)
			preds.push_back(eLab->getPos());
		if (!eLab->isNotAtomic() && lab->getPPoRfView().contains(eLab->getPos()))
			preds.push_back(eLab->getPos());
	}
	return preds;
}

std::vector<Event> LKMMDriver::getMarkedReadSuccs(const EventLabel *lab)
{
	auto &g = getGraph();
	std::vector<Event> succs;

	if (!lab->isNotAtomic())
		succs.push_back(lab->getPos());

	bool rmb = false;
	bool isLabNoRetRead = isNoRetRead(lab);
	for (auto i = lab->getIndex() + 1; i < g.getThreadSize(lab->getThread()); i++) {
		auto *eLab = g.getEventLabel(Event(lab->getThread(), i));
		if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(eLab)) {
			if (fLab->getType() == SmpFenceType::RMB)
				rmb = true;
			continue;
		}
		if (llvm::isa<ReadLabel>(eLab) && !isNoRetRead(eLab) &&
		    !lab->isNotAtomic() && !isLabNoRetRead)
			succs.push_back(eLab->getPos());
	}
	return succs;
}

std::vector<Event> LKMMDriver::getStrongFenceSuccs(Event e)
{
	auto &g = getGraph();
	auto &rcuFence = g.getGlobalRelation(ExecutionGraph::RelationId::rcu_fence);
	auto &allElems = rcuFence.getElems();
	std::vector<Event> succs;

	/* First account for successors in the same thread */
	bool strong = false;
	for (auto i = e.index; i < g.getThreadSize(e.thread); i++) {
		auto *lab = g.getEventLabel(Event(e.thread, i));
		if (auto *fLab = llvm::dyn_cast<SmpFenceLabelLKMM>(lab))
			if (fLab->isStrong())
				strong = true;
		if (auto *fLab = llvm::dyn_cast<RCUSyncLabelLKMM>(lab))
			strong = true;
		if (PROPCalculator::isNonTrivial(lab) && strong)
			succs.push_back(lab->getPos());
	}
	/* Also account for rcu-fence successors */
	for (auto it = rcuFence.adj_begin(e), ie = rcuFence.adj_end(e); it != ie; ++it)
		succs.push_back(allElems[*it]);

	/* Remove duplicates */
	std::sort(succs.begin(), succs.end());
	auto last = std::unique(succs.begin(), succs.end());
	succs.erase(last, succs.end());
	return succs;
}

bool LKMMDriver::isVisConnected(Event a, Event b)
{
	auto &g = getGraph();
	auto &xb = g.getGlobalRelation(ExecutionGraph::RelationId::xb);
	BUG_ON(g.getEventLabel(b)->isNotAtomic());

	if (a.thread == b.thread && !g.getEventLabel(a)->isNotAtomic() && xb(a, b))
		return true;

	auto sfenceSuccsA = getStrongFenceSuccs(a);
	return std::any_of(sfenceSuccsA.begin(), sfenceSuccsA.end(),
			   [&](Event e){
				   auto *eLab = g.getEventLabel(e);
				   return !eLab->isNotAtomic() && xb(e, b);
			   });
}

bool LKMMDriver::isVisBefore(Event a, Event b)
{
	auto &g = getGraph();
	auto &xb = g.getGlobalRelation(ExecutionGraph::RelationId::xb);
	auto &elems = xb.getElems();

	if (isVisConnected(a, b))
		return true;

	auto *mLabA = llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(a));
	for (auto e : elems) {
		if (e != b && !isVisConnected(e, b))
			continue;

		auto *eLab = llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(e));
		if (!eLab)
			continue;
		if (eLab->getFenceView().contains(a))
			return true;
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(eLab)) {
			if (rLab->getRf() == a)
				return true;
			auto *rfLab = g.getEventLabel(rLab->getRf());
			if (auto *rfmLab = llvm::dyn_cast<WriteLabel>(rfLab))
				if (rfmLab->getFenceView().contains(mLabA->getPos()))
					return true;
		}
	}
	return false;
}

bool LKMMDriver::isWWVisBefore(const MemAccessLabel *labA, const MemAccessLabel *labB)
{
	if (isFenceBefore(labA->getPos(), labB->getPos()))
		return true;

	auto &g = getGraph();
	auto &xb = g.getGlobalRelation(ExecutionGraph::RelationId::xb);

	auto markedSuccsA = getMarkedWriteSuccs(labA);
	auto sfenceSuccsA = getStrongFenceSuccs(labA->getPos());
	auto markedPredsB = getMarkedWritePreds(labB);

	/* If any of A's sfence-succs is xb-before some marked-pred of B we have ww-vis */
	for (auto e : sfenceSuccsA) {
		auto *eLab = g.getEventLabel(e);
		if (!eLab->isNotAtomic() && std::any_of(markedPredsB.begin(), markedPredsB.end(),
							[&](Event p){ return xb(e, p); }))
			return true;
	}

	/* If any of A's marked-succs is vis-before some marked-pred of B we have ww-vis */
	for (auto e : markedSuccsA) {
		auto *eLab = g.getEventLabel(e);
		if (!eLab->isNotAtomic() && std::any_of(markedPredsB.begin(), markedPredsB.end(),
							[&](Event p){ return isVisBefore(e, p); }))
			return true;
	}
	return false;
}

bool LKMMDriver::isWRVisBefore(const MemAccessLabel *labA, const MemAccessLabel *labB)
{
	if (isFenceBefore(labA->getPos(), labB->getPos()))
		return true;

	auto &g = getGraph();
	auto &xb = g.getGlobalRelation(ExecutionGraph::RelationId::xb);

	auto markedSuccsA = getMarkedWriteSuccs(labA);
	auto sfenceSuccsA = getStrongFenceSuccs(labA->getPos());
	auto markedPredsB = getMarkedReadPreds(labB);

	/* If any of A's sfence-succs is xb-before some marked-pred of B we have ww-vis */
	for (auto e : sfenceSuccsA) {
		auto *eLab = g.getEventLabel(e);
		if (!eLab->isNotAtomic() && std::any_of(markedPredsB.begin(), markedPredsB.end(),
							[&](Event p){ return xb(e, p); }))
			return true;
	}

	/* If any of A's marked-succs is vis-before some marked-pred of B we have ww-vis */
	for (auto e : markedSuccsA) {
		auto *eLab = g.getEventLabel(e);
		if (!eLab->isNotAtomic() && std::any_of(markedPredsB.begin(), markedPredsB.end(),
							[&](Event p){ return isVisBefore(e, p); }))
			return true;
	}
	return false;
}

bool LKMMDriver::isRWXbBefore(const MemAccessLabel *labA, const MemAccessLabel *labB)
{
	if (isFenceBefore(labA->getPos(), labB->getPos()))
		return true;

	auto &g = getGraph();
	auto &xb = g.getGlobalRelation(ExecutionGraph::RelationId::xb);

	auto markedSuccsA = getMarkedReadSuccs(labA);
	auto markedPredsB = getMarkedWritePreds(labB);

	/* If any of A's marked-succs is vis-before some marked-pred of B we have rw-xb */
	for (auto e : markedSuccsA) {
		auto *eLab = g.getEventLabel(e);
		if (!eLab->isNotAtomic() && std::any_of(markedPredsB.begin(), markedPredsB.end(),
							[&](Event p){ return xb(e, p); }))
			return true;
	}
	return false;
}

bool LKMMDriver::isWRXbBefore(const MemAccessLabel *labA, const MemAccessLabel *labB)
{
	if (isFenceBefore(labA->getPos(), labB->getPos()))
		return true;

	auto &g = getGraph();
	auto &xb = g.getGlobalRelation(ExecutionGraph::RelationId::xb);

	auto markedSuccsA = getMarkedWriteSuccs(labA);
	auto markedPredsB = getMarkedReadPreds(labB);

	/* If any of A's marked-succs is vis-before some marked-pred of B we have wr-xb */
	for (auto e : markedSuccsA) {
		auto *eLab = g.getEventLabel(e);
		if (!eLab->isNotAtomic() && std::any_of(markedPredsB.begin(), markedPredsB.end(),
							[&](Event p){ return xb(e, p); }))
			return true;
	}
	return false;
}

bool LKMMDriver::isValidWWRace(const WriteLabel *labA, const WriteLabel *labB)
{
	auto &g = getGraph();
	auto &co = g.getPerLocRelation(ExecutionGraph::RelationId::co)[labA->getAddr()];

	/* If (a, b) \nin co then this is definitely not a valid race */
	if (!co(labA->getPos(), labB->getPos()))
		return false;

	/* If (a, b) \nin ww-vis this is definitely a race */
	if (!isWWVisBefore(labA, labB))
		return true;

	/* If a or b are plain accesses, we also have to check that they are r-bounded */
	if (!labA->isNotAtomic() && labB->isNotAtomic())
		return !isWRVisBefore(labA, labB);
	else if (labA->isNotAtomic() && !labB->isNotAtomic())
		return !isRWXbBefore(labA, labB);
	else
		return !(isWRVisBefore(labA, labB) && isRWXbBefore(labA, labB));
	BUG();
}

bool LKMMDriver::isValidWRRace(const WriteLabel *labA, const ReadLabel *labB)
{
	auto &g = getGraph();
	auto owrs = getOverwrites(labA);

	/* If (a, b) \nin (co?; rf) then the race is invalid */
	if (labB->getRf() != labA->getPos() &&
	    std::all_of(owrs.begin(), owrs.end(), [&](Event s){ return labB->getRf() != s; }))
		return false;

	/* Otherwise, if one the following holds, then (a, b) \nin race */
	if (isWRVisBefore(labA, labB))
	    return false;
	if (isWRXbBefore(labA, labB))
	    return false;

	return true;
}

bool LKMMDriver::isValidRWRace(const ReadLabel *labA, const WriteLabel *labB)
{
	auto &g = getGraph();
	auto owrs = getOverwrites(labA);

	/* If (a, b) \nin fr then this is not a valid race */
	if (std::find(owrs.begin(), owrs.end(), labB->getPos()) == owrs.end())
		return false;

	return !isRWXbBefore(labA, labB);
}

bool LKMMDriver::isValidRace(Event a, Event b)
{
	auto &g = getGraph();
	auto *labA = llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(a));
	auto *labB = llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(b));
	BUG_ON(!labA || !labB);

	if (auto *wLabA = llvm::dyn_cast<WriteLabel>(labA)) {
		if (auto *wLabB = llvm::dyn_cast<WriteLabel>(labB)) {
			return isValidWWRace(wLabA, wLabB) || isValidWWRace(wLabB, wLabA);
		} else {
			auto *rLabB = static_cast<const ReadLabel *>(labB);
			return isValidWRRace(wLabA, rLabB) || isValidRWRace(rLabB, wLabA);
		}
	} else { /* labA _has to_ be a read => labB _has to_ be a write */
		auto *rLabA = static_cast<const ReadLabel *>(labA);
		auto *wLabB = static_cast<const WriteLabel *>(labB);
		return isValidRWRace(rLabA, wLabB) || isValidWRRace(wLabB, rLabA);
	}
	BUG();
	return false;
}

Event LKMMDriver::findDataRaceForMemAccess(const MemAccessLabel *mLab)
{
	std::vector<Event> potential;

	if (auto *rLab = llvm::dyn_cast<ReadLabel>(mLab))
		potential = findPotentialRacesForNewLoad(rLab);
	else if (auto *wLab = llvm::dyn_cast<WriteLabel>(mLab))
		potential = findPotentialRacesForNewStore(wLab);
	else
		BUG();

	/* If no potential races found, exit early */
	if (potential.empty())
		return Event::getInitializer();

	/* Otherwise, first make sure the graph is consistent.
	 * We do this prematurely because the validity of a race
	 * assumes (among others) that xb is already calculated */
	if (!isConsistent(ProgramPoint::error)) {
		for (auto i = 0u; i < getGraph().getNumThreads(); i++)
			getEE()->getThrById(i).block();
		return Event::getInitializer();
	}

	for (auto p : potential)
		if (isValidRace(mLab->getPos(), p))
			return p;
	return Event::getInitializer();
}

std::vector<Event> LKMMDriver::getStoresToLoc(const llvm::GenericValue *addr)
{
	return getGraph().getCoherentStores(addr, getEE()->getCurrentPosition());
}

std::vector<Event> LKMMDriver::getRevisitLoads(const WriteLabel *sLab)
{
	return getGraph().getCoherentRevisits(sLab);
}

void LKMMDriver::changeRf(Event read, Event store)
{
	auto &g = getGraph();

	/* Change the reads-from relation in the graph */
	g.changeRf(read, store);

	/* And update the views of the load */
	EventLabel *lab = g.getEventLabel(read);
	ReadLabel *rLab = static_cast<ReadLabel *>(lab);
	View hb = calcBasicHbView(rLab->getPos());
	DepView pporf(rLab->getPPoView());

	updateReadViewsFromRf(pporf, hb, rLab);

	rLab->setHbView(std::move(hb));
	rLab->setPPoRfView(std::move(pporf));
}

bool LKMMDriver::updateJoin(Event join, Event childLast)
{
       	auto &g = getGraph();

       	if (!g.updateJoin(join, childLast))
       		return false;

       	EventLabel *jLab = g.getEventLabel(join);
       	EventLabel *fLab = g.getEventLabel(childLast);

       /* Since the pporf view may contain elements from threads joined
       	* in previous explorations, we have to reset it to the ppo one,
       	* and then update it */
       	DepView pporf(jLab->getPPoView());
       	View hb = calcBasicHbView(jLab->getPos());

       	hb.update(fLab->getHbView());
       	pporf.update(fLab->getPPoRfView());

        jLab->setHbView(std::move(hb));
       	jLab->setPPoRfView(std::move(pporf));
       	return true;
}

void LKMMDriver::initConsCalculation()
{
	return;
}
