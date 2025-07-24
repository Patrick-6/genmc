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

#include "ExecutionGraph/GraphUtils.hpp"
#include "ExecutionGraph/EventLabel.hpp"
#include "ExecutionGraph/ExecutionGraph.hpp"
#include "ExecutionGraph/GraphIterators.hpp"
#include <algorithm>
#include <llvm/Support/Casting.h>
#include <ranges>

auto isHazptrProtected(const MemAccessLabel *mLab) -> bool
{
	auto &g = *mLab->getParent();
	BUG_ON(!mLab->getAddr().isDynamic());

	auto *aLab = mLab->getAlloc();
	BUG_ON(!aLab);

	auto mpreds = po_preds(g, mLab);
	auto pLabIt = std::ranges::find_if(mpreds, [&](auto &lab) {
		auto *pLab = llvm::dyn_cast<HpProtectLabel>(&lab);
		return pLab && aLab->contains(pLab->getProtectedAddr());
	});
	if (pLabIt == mpreds.end() || !llvm::isa<HpProtectLabel>(&*pLabIt))
		return false;

	auto *pLab = llvm::dyn_cast<HpProtectLabel>(&*pLabIt);
	for (auto &lab : std::ranges::subrange(std::ranges::begin(mpreds), pLabIt)) {
		if (auto *oLab = llvm::dyn_cast<HpProtectLabel>(&lab))
			if (oLab->getHpAddr() == pLab->getHpAddr())
				return false;
	}
	return true;
}

auto findMatchingLock(const UnlockWriteLabel *uLab) -> const CasWriteLabel *
{
	const auto &g = *uLab->getParent();
	std::vector<const UnlockWriteLabel *> locUnlocks;

	for (auto &lab : g.po_preds(uLab)) {

		/* In case support for reentrant locks is added... */
		if (auto *suLab = llvm::dyn_cast<UnlockWriteLabel>(&lab)) {
			if (suLab->getAddr() == uLab->getAddr())
				locUnlocks.push_back(suLab);
		}
		if (auto *lLab = llvm::dyn_cast<CasWriteLabel>(&lab)) {
			if ((llvm::isa<LockCasWriteLabel>(lLab) ||
			     llvm::isa<TrylockCasWriteLabel>(lLab)) &&
			    lLab->getAddr() == uLab->getAddr()) {
				if (locUnlocks.empty())
					return lLab;
				locUnlocks.pop_back();
			}
		}
	}
	return nullptr;
}

auto findMatchingUnlock(const CasWriteLabel *lLab) -> const UnlockWriteLabel *
{
	const auto &g = *lLab->getParent();
	std::vector<const CasWriteLabel *> locLocks;

	BUG_ON(!llvm::isa<LockCasReadLabel>(lLab) && !llvm::isa<TrylockCasReadLabel>(lLab));
	for (auto &lab : g.po_succs(lLab)) {
		/* skip next event */

		/* In case support for reentrant locks is added... */
		if (auto *slLab = llvm::dyn_cast<CasWriteLabel>(&lab)) {
			if ((llvm::isa<LockCasWriteLabel>(slLab) ||
			     llvm::isa<TrylockCasWriteLabel>(slLab)) &&
			    slLab->getAddr() == lLab->getAddr())
				locLocks.push_back(slLab);
		}
		if (auto *uLab = llvm::dyn_cast<UnlockWriteLabel>(&lab)) {
			if (uLab->getAddr() == lLab->getAddr()) {
				if (locLocks.empty())
					return uLab;
				locLocks.pop_back();
			}
		}
	}
	return nullptr;
}

auto findMatchingSpeculativeRead(const ReadLabel *cLab, const EventLabel *&scLab)
	-> const SpeculativeReadLabel *
{
	const auto &g = *cLab->getParent();
	for (auto &lab : g.po_preds(cLab)) {

		if (lab.isSC())
			scLab = &lab;

		/* We don't care whether all previous confirmations are matched;
		 * the same speculation maybe confirmed multiple times (e.g., baskets) */
		if (auto *rLab = llvm::dyn_cast<SpeculativeReadLabel>(&lab)) {
			if (rLab->getAddr() == cLab->getAddr())
				return rLab;
		}
	}
	return nullptr;
}

auto findAllocatingLabel(const ExecutionGraph &g, const SAddr &addr) -> const MallocLabel *
{
	/* Don't bother for static addresses */
	if (!addr.isDynamic())
		return nullptr;

	/* Fastpath: location contains a store */
	if (g.containsLoc(addr) && g.co_max(addr) != g.getInitLabel())
		return llvm::cast<WriteLabel>(g.co_max(addr))->getAlloc();

	/* Iterate over labels */
	auto labels = g.rlabels();
	auto labIt = std::ranges::find_if(labels, [addr](auto &lab) {
		auto *mLab = llvm::dyn_cast<MallocLabel>(&lab);
		return mLab && mLab->contains(addr);
	});
	return (labIt != std::ranges::end(labels)) ? llvm::dyn_cast<MallocLabel>(&*labIt) : nullptr;
}

auto findAllocatingLabel(ExecutionGraph &g, const SAddr &addr) -> MallocLabel *
{
	return const_cast<MallocLabel *>(
		findAllocatingLabel(static_cast<const ExecutionGraph &>(g), addr));
}

static auto getMinimumStampLabel(const std::vector<const WriteLabel *> &labs) -> const WriteLabel *
{
	auto minIt = std::ranges::min_element(
		labs, [&](auto &lab1, auto &lab2) { return lab1->getStamp() < lab2->getStamp(); });
	return minIt == std::ranges::end(labs) ? nullptr : *minIt;
}

auto findPendingRMW(const WriteLabel *sLab) -> const WriteLabel *
{
	if (!sLab->isRMW())
		return nullptr;

	const auto &g = *sLab->getParent();
	const auto *pLab = llvm::dyn_cast<ReadLabel>(g.po_imm_pred(sLab));
	BUG_ON(!pLab->getRf());
	std::vector<const WriteLabel *> pending;

	/* Fastpath: non-init rf */
	if (auto *wLab = llvm::dyn_cast<WriteLabel>(pLab->getRf())) {
		for (auto &rLab : wLab->readers()) {
			if (rLab.isRMW() && &rLab != pLab)
				pending.push_back(llvm::dyn_cast<WriteLabel>(g.po_imm_succ(&rLab)));
		}
		return getMinimumStampLabel(pending);
	}

	/* Slowpath: scan init rfs */
	std::for_each(
		g.init_rf_begin(pLab->getAddr()), g.init_rf_end(pLab->getAddr()), [&](auto &rLab) {
			if (rLab.getRf() == pLab->getRf() && &rLab != pLab && rLab.isRMW())
				pending.push_back(llvm::dyn_cast<WriteLabel>(g.po_imm_succ(&rLab)));
		});
	return getMinimumStampLabel(pending);
}

auto findPendingRMW(WriteLabel *sLab) -> WriteLabel *
{
	return const_cast<WriteLabel *>(findPendingRMW(static_cast<const WriteLabel *>(sLab)));
}

auto findBarrierInitValue(const BIncFaiWriteLabel *wLab) -> SVal
{
	const auto &g = *wLab->getParent();
	const auto *irLab = llvm::dyn_cast<ReadLabel>(g.po_imm_pred(g.po_imm_pred(wLab)));
	BUG_ON(!irLab || !irLab->isNotAtomic());

	return irLab->getReturnValue();
}

auto isLastInBarrierRound(const BIncFaiWriteLabel *wLab) -> bool
{
	return wLab->getVal() % findBarrierInitValue(wLab) == SVal(0);
}

auto readsBarrierUnblockingValue(const BWaitReadLabel *rLab) -> bool
{
	const auto &g = *rLab->getParent();
	const auto *wLab = llvm::cast<BIncFaiWriteLabel>(g.po_imm_pred(rLab));
	auto iVal = g.po_imm_pred(g.po_imm_pred(wLab))->getReturnValue();
	auto val = rLab->getReturnValue();

	return val.uge(wLab->getVal() / iVal * iVal +
		       (isLastInBarrierRound(wLab) ? SVal(0) : iVal));
}

auto violatesAtomicity(const WriteLabel *sLab) -> bool
{
	return sLab->isRMW() && findPendingRMW(sLab);
}

void blockThread(ExecutionGraph &g, std::unique_ptr<BlockLabel> bLab)
{
	/* There are a couple of reasons we don't call Driver::addLabelToGraph() here:
	 *   1) It's redundant to update the views of the block label
	 *   2) If addLabelToGraph() does extra stuff (e.g., event caching) we absolutely
	 *      don't want to do that here. blockThread() should be safe to call from
	 *      anywhere in the code, with no unexpected side-effects */
	if (bLab->getPos() == g.getLastThreadLabel(bLab->getThread())->getPos())
		g.removeLast(bLab->getThread());
	g.addLabelToGraph(std::move(bLab));
}

void unblockThread(ExecutionGraph &g, Event pos)
{
	auto *bLab = g.getLastThreadLabel(pos.thread);
	BUG_ON(!llvm::isa<BlockLabel>(bLab));
	g.removeLast(pos.thread);
}

auto createRMWWriteLabel(const ExecutionGraph &g, const ReadLabel *rLab)
	-> std::unique_ptr<WriteLabel>
{
	/* Handle non-RMW cases first */
	if (!llvm::isa<CasReadLabel>(rLab) && !llvm::isa<FaiReadLabel>(rLab))
		return nullptr;
	if (auto *casLab = llvm::dyn_cast<CasReadLabel>(rLab))
		if (rLab->getAccessValue(rLab->getAccess()) != casLab->getExpected())
			return nullptr;

	SVal result;
	WriteAttr wattr = WriteAttr::None;
	if (auto *faiLab = llvm::dyn_cast<FaiReadLabel>(rLab)) {
		/* Need to get the rf value within the if, as rLab might be a disk op,
		 * and we cannot get the value in that case (but it will also not be an RMW)
		 */
		auto rfVal = rLab->getAccessValue(rLab->getAccess());
		result = executeRMWBinOp(rfVal, faiLab->getOpVal(), faiLab->getSize(),
					 faiLab->getOp());
		wattr = faiLab->getAttr();
	} else if (auto *casLab = llvm::dyn_cast<CasReadLabel>(rLab)) {
		result = casLab->getSwapVal();
		wattr = casLab->getAttr();
	} else
		BUG();

	std::unique_ptr<WriteLabel> wLab = nullptr;

#define CREATE_COUNTERPART(name)                                                                   \
	case EventLabel::name##Read:                                                               \
		wLab = name##WriteLabel::create(rLab->getPos().next(), rLab->getOrdering(),        \
						rLab->getAddr(), rLab->getSize(), rLab->getType(), \
						result, wattr);                                    \
		break;

	switch (rLab->getKind()) {
		CREATE_COUNTERPART(BIncFai);
		CREATE_COUNTERPART(NoRetFai);
		CREATE_COUNTERPART(Fai);
		CREATE_COUNTERPART(LockCas);
		CREATE_COUNTERPART(TrylockCas);
		CREATE_COUNTERPART(AbstractLockCas);
		CREATE_COUNTERPART(Cas);
		CREATE_COUNTERPART(HelpedCas);
		CREATE_COUNTERPART(ConfirmingCas);
	default:
		BUG();
	}
	BUG_ON(!wLab);
	return std::move(wLab);
}
