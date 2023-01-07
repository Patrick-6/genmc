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
#include "DepExecutionGraph.hpp"

std::vector<Event>
DepExecutionGraph::getRevisitable(const WriteLabel *sLab) const
{
	auto pendingRMW = getPendingRMW(sLab);
	std::vector<Event> loads;

	for (auto i = 0u; i < getNumThreads(); i++) {
		if (sLab->getThread() == i)
			continue;
		for (auto j = 1u; j < getThreadSize(i); j++) {
			const EventLabel *lab = getEventLabel(Event(i, j));
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
				if (rLab->getAddr() == sLab->getAddr() &&
				    !sLab->getPPoRfView().contains(rLab->getPos()) &&
				    rLab->isRevisitable() && rLab->wasAddedMax())
					loads.push_back(rLab->getPos());
			}
		}
	}
	if (!pendingRMW.isInitializer())
		loads.erase(std::remove_if(loads.begin(), loads.end(), [&](Event &e){
			auto *confLab = getEventLabel(pendingRMW);
			return getEventLabel(e)->getStamp() > confLab->getStamp();
		}), loads.end());
	return loads;
}

void updatePredsWithPrefixView(const DepExecutionGraph &g, DepView &preds, const DepView &pporf)
{
	/* In addition to taking (preds U pporf), make sure pporf includes rfis */
	preds.update(pporf);
	for (auto i = 0u; i < pporf.size(); i++) {
		for (auto j = 1; j <= pporf.getMax(i); j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
				if (preds.contains(rLab->getPos()) && !preds.contains(rLab->getRf())) {
					if (rLab->getRf().thread == rLab->getThread())
						preds.removeHole(rLab->getRf());
				}
			}
		}
	}
	return;
}

std::unique_ptr<VectorClock>
DepExecutionGraph::getRevisitView(const BackwardRevisit &r) const
{
	auto *rLab = getReadLabel(r.getPos());
	auto preds = getViewFromStamp(rLab->getStamp());

	updatePredsWithPrefixView(*this, *llvm::dyn_cast<DepView>(&*preds),
				  getWriteLabel(r.getRev())->getPPoRfView());
	if (auto *br = llvm::dyn_cast<BackwardRevisitHELPER>(&r))
		updatePredsWithPrefixView(*this, *llvm::dyn_cast<DepView>(&*preds),
					  getWriteLabel(br->getMid())->getPPoRfView());
	return std::move(preds);
}

std::unique_ptr<VectorClock>
DepExecutionGraph::getViewFromStamp(Stamp stamp) const
{
	auto preds = std::make_unique<DepView>();

	for (auto i = 0u; i < getNumThreads(); i++) {
		for (auto j = 1u; j < getThreadSize(i); j++) {
			const EventLabel *lab = getEventLabel(Event(i, j));
			if (lab->getStamp() <= stamp)
				preds->setMax(Event(i, j));
		}
	}
	return preds;
}

bool DepExecutionGraph::revisitModifiesGraph(const BackwardRevisit &r) const
{
	auto v = getRevisitView(r);

	for (auto i = 0u; i < getNumThreads(); i++) {
		if (v->getMax(i) + 1 != (long) getThreadSize(i) &&
		    !getEventLabel(Event(i, v->getMax(i) + 1))->isTerminator())
			return true;
		for (auto j = 0u; j < getThreadSize(i); j++) {
			const EventLabel *lab = getEventLabel(Event(i, j));
			if (!v->contains(lab->getPos()) && !llvm::isa<EmptyLabel>(lab) &&
			    !lab->isTerminator())
				return true;
		}
	}
	return false;
}

bool isFixedHoleInView(const DepExecutionGraph &g, const EventLabel *lab, const DepView &v)
{
	if (auto *wLabB = llvm::dyn_cast<WriteLabel>(lab))
		return std::any_of(wLabB->getReadersList().begin(), wLabB->getReadersList().end(),
				   [&v](const Event &r){ return v.contains(r); });

	auto *rLabB = llvm::dyn_cast<ReadLabel>(lab);
	if (!rLabB)
		return false;

	/* If prefix has same address load, we must read from the same write */
	for (auto i = 0u; i < v.size(); i++) {
		for (auto j = 0u; j <= v.getMax(i); j++) {
			if (!v.contains(Event(i, j)))
				continue;
			if (auto *mLab = g.getReadLabel(Event(i, j)))
				if (mLab->getAddr() == rLabB->getAddr() && mLab->getRf() == rLabB->getRf())
					return true;
		}
	}

	if (g.isRMWLoad(rLabB)) {
		auto *wLabB = g.getWriteLabel(rLabB->getPos().next());
		return std::any_of(wLabB->getReadersList().begin(), wLabB->getReadersList().end(),
				   [&v](const Event &r){ return v.contains(r); });
	}
	return false;
}

bool DepExecutionGraph::prefixContainsSameLoc(const BackwardRevisit &r,
					      const EventLabel *lab) const
{
	/* Some holes need to be treated specially. However, it is _wrong_ to keep
	 * porf views around. What we should do instead is simply check whether
	 * an event is "part" of WLAB's pporf view (even if it is not contained in it).
	 * Similar actions are taken in {WB,MO}Calculator */
	auto &v = getWriteLabel(r.getRev())->getPPoRfView();
	if (lab->getIndex() <= v.getMax(lab->getThread()) && isFixedHoleInView(*this, lab, v))
		return true;
	if (auto *br = llvm::dyn_cast<BackwardRevisitHELPER>(&r)) {
		auto &hv = getWriteLabel(br->getMid())->getPPoRfView();
		return lab->getIndex() <= hv.getMax(lab->getThread()) && isFixedHoleInView(*this, lab, hv);
	}
	return false;
}

void DepExecutionGraph::cutToStamp(Stamp stamp)
{
	/* First remove events from the modification order */
	auto preds = getViewFromStamp(stamp);

	/* Inform all calculators about the events cutted */
	auto &calcs = getCalcs();
	for (auto i = 0u; i < calcs.size(); i++)
		calcs[i]->removeAfter(*preds);

	/* Then, restrict the graph */
	for (auto i = 0u; i < getNumThreads(); i++) {
		/* We reset the maximum index for this thread, as we
		 * do not know the order in which events were added */
		auto newMax = 1u;
		for (auto j = 1u; j < getThreadSize(i); j++) { /* Keeps begins */
			const EventLabel *lab = getEventLabel(Event(i, j));
			/* If this label should not be deleted, check if it
			 * is gonna be the new maximum, and then skip */
			if (lab->getStamp() <= stamp) {
				if (lab->getIndex() >= newMax)
					newMax = lab->getIndex() + 1;
				continue;
			}

			/* Otherwise, remove 'pointers' to it, in an
			 * analogous manner cutToView(). */
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
				Event rf = rLab->getRf();
				/* Make sure RF label exists */
				EventLabel *rfLab = getEventLabel(rf);
				if (rfLab) {
					if (auto *wLab = llvm::dyn_cast<WriteLabel>(rfLab)) {
						wLab->removeReader([&](Event r){
								return r == rLab->getPos();
							});
					}
				}
			}
			setEventLabel(Event(i, j), nullptr);
		}
		resizeThread(i, newMax);
	}

	/* Finally, do not keep any nullptrs in the graph */
	for (auto i = 0u; i < getNumThreads(); i++) {
		for (auto j = 0u; j < getThreadSize(i); j++) {
			if (getEventLabel(Event(i, j)))
				continue;
			setEventLabel(Event(i, j), EmptyLabel::create(Event(i, j)));
			getEventLabel(Event(i, j))->setStamp(nextStamp());
		}
	}
}

std::unique_ptr<ExecutionGraph>
DepExecutionGraph::getCopyUpTo(const VectorClock &v) const
{
	auto og = std::unique_ptr<DepExecutionGraph>(new DepExecutionGraph());
	copyGraphUpTo(*og, v);
	return og;
}
