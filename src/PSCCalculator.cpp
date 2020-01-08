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

#include "PSCCalculator.hpp"

std::pair<std::vector<Event>, std::vector<Event> >
PSCCalculator::getSCs() const
{
	std::vector<Event> scs, fcs;

	for (auto i = 0u; i < getNumThreads(); i++) {
		for (auto j = 0u; j < getThreadSize(i); j++) {
			const EventLabel *lab = getEventLabel(Event(i, j));
			if (lab->isSC() && !isRMWLoad(lab))
				scs.push_back(lab->getPos());
			if (lab->isSC() && llvm::isa<FenceLabel>(lab))
				fcs.push_back(lab->getPos());
		}
	}
	return std::make_pair(scs,fcs);
}

std::vector<const llvm::GenericValue *> PSCCalculator::getDoubleLocs() const
{
	std::vector<const llvm::GenericValue *> singles, doubles;

	for (auto i = 0u; i < getNumThreads(); i++) {
		for (auto j = 1u; j < getThreadSize(i); j++) { /* Do not consider thread inits */
			const EventLabel *lab = getEventLabel(Event(i, j));
			if (!llvm::isa<MemAccessLabel>(lab))
				continue;

			auto *mLab = static_cast<const MemAccessLabel *>(lab);
			if (std::find(doubles.begin(), doubles.end(),
				      mLab->getAddr()) != doubles.end())
				continue;
			if (std::find(singles.begin(), singles.end(),
				      mLab->getAddr()) != singles.end()) {
				singles.erase(std::remove(singles.begin(),
							  singles.end(),
							  mLab->getAddr()),
					      singles.end());
				doubles.push_back(mLab->getAddr());
			} else {
				singles.push_back(mLab->getAddr());
			}
		}
	}
	return doubles;
}

std::vector<Event> PSCCalculator::calcSCFencesSuccs(const std::vector<Event> &fcs,
						     const Event e) const
{
	std::vector<Event> succs;

	if (isRMWLoad(e))
		return succs;
	for (auto &f : fcs) {
		if (getHbBefore(f).contains(e))
			succs.push_back(f);
	}
	return succs;
}

std::vector<Event> PSCCalculator::calcSCFencesPreds(const std::vector<Event> &fcs,
						     const Event e) const
{
	std::vector<Event> preds;
	auto &before = getHbBefore(e);

	if (isRMWLoad(e))
		return preds;
	for (auto &f : fcs) {
		if (before.contains(f))
			preds.push_back(f);
	}
	return preds;
}

std::vector<Event> PSCCalculator::calcSCSuccs(const std::vector<Event> &fcs,
					       const Event e) const
{
	const EventLabel *lab = getEventLabel(e);

	if (isRMWLoad(lab))
		return {};
	if (lab->isSC())
		return {e};
	else
		return calcSCFencesSuccs(fcs, e);
}

std::vector<Event> PSCCalculator::calcSCPreds(const std::vector<Event> &fcs,
					       const Event e) const
{
	const EventLabel *lab = getEventLabel(e);

	if (isRMWLoad(lab))
		return {};
	if (lab->isSC())
		return {e};
	else
		return calcSCFencesPreds(fcs, e);
}

std::vector<Event> PSCCalculator::calcRfSCSuccs(const std::vector<Event> &fcs,
						 const Event ev) const
{
	const EventLabel *lab = getEventLabel(ev);
	std::vector<Event> rfs;

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *wLab = static_cast<const WriteLabel *>(lab);
	for (const auto &e : wLab->getReadersList()) {
		auto succs = calcSCSuccs(fcs, e);
		rfs.insert(rfs.end(), succs.begin(), succs.end());
	}
	return rfs;
}

std::vector<Event> PSCCalculator::calcRfSCFencesSuccs(const std::vector<Event> &fcs,
						       const Event ev) const
{
	const EventLabel *lab = getEventLabel(ev);
	std::vector<Event> fenceRfs;

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *wLab = static_cast<const WriteLabel *>(lab);
	for (const auto &e : wLab->getReadersList()) {
		auto fenceSuccs = calcSCFencesSuccs(fcs, e);
		fenceRfs.insert(fenceRfs.end(), fenceSuccs.begin(), fenceSuccs.end());
	}
	return fenceRfs;
}

void PSCCalculator::addRbEdges(const std::vector<Event> &fcs,
				const std::vector<Event> &moAfter,
				const std::vector<Event> &moRfAfter,
				Matrix2D<Event> &matrix,
				const Event &ev) const
{
	const EventLabel *lab = getEventLabel(ev);

	BUG_ON(!llvm::isa<WriteLabel>(lab));
	auto *wLab = static_cast<const WriteLabel *>(lab);
	for (const auto &e : wLab->getReadersList()) {
		auto preds = calcSCPreds(fcs, e);
		auto fencePreds = calcSCFencesPreds(fcs, e);

		matrix.addEdgesFromTo(preds, moAfter);        /* Base/fence: Adds rb-edges */
		matrix.addEdgesFromTo(fencePreds, moRfAfter); /* Fence: Adds (rb;rf)-edges */
	}
	return;
}

void PSCCalculator::addMoRfEdges(const std::vector<Event> &fcs,
				  const std::vector<Event> &moAfter,
				  const std::vector<Event> &moRfAfter,
				  Matrix2D<Event> &matrix,
				  const Event &ev) const
{
	auto preds = calcSCPreds(fcs, ev);
	auto fencePreds = calcSCFencesPreds(fcs, ev);
	auto rfs = calcRfSCSuccs(fcs, ev);

	matrix.addEdgesFromTo(preds, moAfter);        /* Base/fence:  Adds mo-edges */
	matrix.addEdgesFromTo(preds, rfs);            /* Base/fence:  Adds rf-edges (hb_loc) */
	matrix.addEdgesFromTo(fencePreds, moRfAfter); /* Fence:       Adds (mo;rf)-edges */
	return;
}

/*
 * addSCEcos - Helper function that calculates a part of PSC_base and PSC_fence
 *
 * For PSC_base and PSC_fence, it adds mo, rb, and hb_loc edges. The
 * procedure for mo and rb is straightforward: at each point, we only
 * need to keep a list of all the mo-after writes that are either SC,
 * or can reach an SC fence. For hb_loc, however, we only consider
 * rf-edges because the other cases are implicitly covered (sb, mo, etc).
 *
 * For PSC_fence only, it adds (mo;rf)- and (rb;rf)-edges. Simple cases like
 * mo, rf, and rb are covered by PSC_base, and all other combinations with
 * more than one step either do not compose, or lead to an already added
 * single-step relation (e.g, (rf;rb) => mo, (rb;mo) => rb)
 */
void PSCCalculator::addSCEcos(const std::vector<Event> &fcs,
			       const std::vector<Event> &mo,
			       Matrix2D<Event> &matrix) const
{
	std::vector<Event> moAfter;   /* mo-after SC writes or SC fences reached by an mo-after write */
	std::vector<Event> moRfAfter; /* SC fences that can be reached by (mo;rf)-after reads */

	for (auto rit = mo.rbegin(); rit != mo.rend(); rit++) {

		/* First, add edges to SC events that are (mo U rb);rf?-after this write */
		addRbEdges(fcs, moAfter, moRfAfter, matrix, *rit);
		addMoRfEdges(fcs, moAfter, moRfAfter, matrix, *rit);

		/* Then, update the lists of mo and mo;rf SC successors */
		auto succs = calcSCSuccs(fcs, *rit);
		auto fenceRfs = calcRfSCFencesSuccs(fcs, *rit);
		moAfter.insert(moAfter.end(), succs.begin(), succs.end());
		moRfAfter.insert(moRfAfter.end(), fenceRfs.begin(), fenceRfs.end());
	}
}

/*
 * Similar to addSCEcos but uses a partial order among stores (WB) to
 * add coherence (mo/wb) and rb edges.
 */
void PSCCalculator::addSCEcos(const std::vector<Event> &fcs,
			       Matrix2D<Event> &wbMatrix,
			       Matrix2D<Event> &pscMatrix) const
{
	auto &stores = wbMatrix.getElems();
	for (auto i = 0u; i < stores.size(); i++) {

		/*
		 * Calculate which of the stores are wb-after the current
		 * write, and then collect wb-after and (wb;rf)-after SC successors
		 */
		std::vector<Event> wbAfter, wbRfAfter;
		for (auto j = 0u; j < stores.size(); j++) {
			if (wbMatrix(i, j)) {
				auto succs = calcSCSuccs(fcs, stores[j]);
				auto fenceRfs = calcRfSCFencesSuccs(fcs, stores[j]);
				wbAfter.insert(wbAfter.end(), succs.begin(), succs.end());
				wbRfAfter.insert(wbRfAfter.end(), fenceRfs.begin(), fenceRfs.end());
			}
		}

		/* Then, add the proper edges to PSC using wb-after and (wb;rf)-after successors */
		addRbEdges(fcs, wbAfter, wbRfAfter, pscMatrix, stores[i]);
		addMoRfEdges(fcs, wbAfter, wbRfAfter, pscMatrix, stores[i]);
	}
}

/*
 * Adds sb as well as [Esc];sb_(<>loc);hb;sb_(<>loc);[Esc] edges. The first
 * part of this function is common for PSC_base and PSC_fence, while the second
 * part of this function is not triggered for fences (these edges are covered in
 * addSCEcos()).
 */
void PSCCalculator::addSbHbEdges(Matrix2D<Event> &matrix) const
{
	auto &scs = matrix.getElems();
	for (auto i = 0u; i < scs.size(); i++) {
		for (auto j = 0u; j < scs.size(); j++) {
			if (i == j)
				continue;
			const EventLabel *eiLab = getEventLabel(scs[i]);
			const EventLabel *ejLab = getEventLabel(scs[j]);

			/* PSC_base/PSC_fence: Adds sb-edges*/
			if (eiLab->getThread() == ejLab->getThread()) {
				if (eiLab->getIndex() < ejLab->getIndex())
					matrix(i, j) = true;
				continue;
			}

			/* PSC_base: Adds [Esc];sb_(<>loc);hb;sb_(<>loc);[Esc] edges.
			 * We do need to consider the [Fsc];hb? cases, since these
			 * will be covered by addSCEcos(). (More speficically, from
			 * the rf/hb_loc case in addMoRfEdges().)  */
			const EventLabel *ejPrevLab = getPreviousNonEmptyLabel(ejLab);
			if (!llvm::isa<MemAccessLabel>(ejPrevLab) ||
			    !llvm::isa<MemAccessLabel>(ejLab) ||
			    !llvm::isa<MemAccessLabel>(eiLab))
				continue;

			if (eiLab->getPos() == getLastThreadEvent(eiLab->getThread()))
				continue;

			auto *ejPrevMLab = static_cast<const MemAccessLabel *>(ejPrevLab);
			auto *ejMLab = static_cast<const MemAccessLabel *>(ejLab);
			auto *eiMLab = static_cast<const MemAccessLabel *>(eiLab);

			if (ejPrevMLab->getAddr() != ejMLab->getAddr()) {
				Event next = eiMLab->getPos().next();
				const EventLabel *eiNextLab = getEventLabel(next);
				if (auto *eiNextMLab =
				    llvm::dyn_cast<MemAccessLabel>(eiNextLab)) {
					if (eiMLab->getAddr() != eiNextMLab->getAddr() &&
					    ejPrevMLab->getHbView().contains(eiNextMLab->getPos()))
						matrix(i, j) = true;
				}
			}
		}
	}
	return;
}

void PSCCalculator::addInitEdges(const std::vector<Event> &fcs,
				  Matrix2D<Event> &matrix) const
{
	for (auto i = 0u; i < getNumThreads(); i++) {
		for (auto j = 0u; j < getThreadSize(i); j++) {
			const EventLabel *lab = getEventLabel(Event(i, j));
			/* Consider only reads that read from the initializer write */
			if (!llvm::isa<ReadLabel>(lab) || isRMWLoad(lab))
				continue;
			auto *rLab = static_cast<const ReadLabel *>(lab);
			if (!rLab->getRf().isInitializer())
				continue;

			auto preds = calcSCPreds(fcs, rLab->getPos());
			auto fencePreds = calcSCFencesPreds(fcs, rLab->getPos());
			for (auto &w : getStoresToLoc(rLab->getAddr())) {
				/* Can be casted to WriteLabel by construction */
				auto *wLab = static_cast<const WriteLabel *>(
					getEventLabel(w));
				auto wSuccs = calcSCSuccs(fcs, w);
				matrix.addEdgesFromTo(preds, wSuccs); /* Adds rb-edges */
				for (auto &r : wLab->getReadersList()) {
					auto fenceSuccs = calcSCFencesSuccs(fcs, r);
					matrix.addEdgesFromTo(fencePreds, fenceSuccs); /*Adds (rb;rf)-edges */
				}
			}
		}
	}
	return;
}

template <typename F>
bool PSCCalculator::addSCEcosMO(const std::vector<Event> &fcs,
				 const std::vector<const llvm::GenericValue *> &scLocs,
				 Matrix2D<Event> &matrix, F cond) const
{
	BUG_ON(!llvm::isa<MOCoherenceCalculator>(getCoherenceCalculator()));
	for (auto loc : scLocs) {
		auto &stores = getStoresToLoc(loc); /* Will already be ordered... */
		addSCEcos(fcs, stores, matrix);
	}
	matrix.transClosure();
	return cond(matrix);
}

template <typename F>
bool PSCCalculator::addSCEcosWBWeak(const std::vector<Event> &fcs,
				     const std::vector<const llvm::GenericValue *> &scLocs,
				     Matrix2D<Event> &matrix, F cond) const
{
	const auto *cc = getCoherenceCalculator();

	BUG_ON(!llvm::isa<WBCoherenceCalculator>(cc));
	auto *cohTracker = static_cast<const WBCoherenceCalculator *>(cc);
	for (auto loc : scLocs) {
		auto wb = cohTracker->calcWb(loc);
		auto sortedStores = wb.topoSort();
		addSCEcos(fcs, sortedStores, matrix);
	}
	matrix.transClosure();
	return cond(matrix);
}

template <typename F>
bool PSCCalculator::addSCEcosWB(const std::vector<Event> &fcs,
				 const std::vector<const llvm::GenericValue *> &scLocs,
				 Matrix2D<Event> &matrix, F cond) const
{
	const auto *cc = getCoherenceCalculator();

	BUG_ON(!llvm::isa<WBCoherenceCalculator>(cc));
	auto *cohTracker = static_cast<const WBCoherenceCalculator *>(cc);
	for (auto loc : scLocs) {
		auto wb = cohTracker->calcWb(loc);
		addSCEcos(fcs, wb, matrix);
	}
	matrix.transClosure();
	return cond(matrix);
}

template <typename F>
bool PSCCalculator::addSCEcosWBFull(const std::vector<Event> &fcs,
				     const std::vector<const llvm::GenericValue *> &scLocs,
				     Matrix2D<Event> &matrix, F cond) const
{
	const auto *cc = getCoherenceCalculator();

	BUG_ON(!llvm::isa<WBCoherenceCalculator>(cc));
	auto *cohTracker = static_cast<const WBCoherenceCalculator *>(cc);

	std::vector<std::vector<std::vector<Event> > > topoSorts(scLocs.size());
	for (auto i = 0u; i < scLocs.size(); i++) {
		auto wb = cohTracker->calcWb(scLocs[i]);
		topoSorts[i] = wb.allTopoSort();
	}

	unsigned int K = topoSorts.size();
	std::vector<unsigned int> count(K, 0);

	/*
	 * It suffices to find one combination for the WB extensions of all
	 * locations, for which PSC is acyclic. This loop is like an odometer:
	 * given an array that contains K vectors, we keep a counter for each
	 * vector, and proceed by incrementing the rightmost counter. Like in
	 * addition, if a carry is created, this is propagated to the left.
	 */
	while (count[0] < topoSorts[0].size()) {
		/* Process current combination */
		auto tentativePSC(matrix);
		for (auto i = 0u; i < K; i++)
			addSCEcos(fcs, topoSorts[i][count[i]], tentativePSC);

		tentativePSC.transClosure();
		if (cond(tentativePSC))
			return true;

		/* Find next combination */
		++count[K - 1];
		for (auto i = K - 1; (i > 0) && (count[i] == topoSorts[i].size()); --i) {
			count[i] = 0;
			++count[i - 1];
		}
	}

	/* No valid MO combination found */
	return false;
}

template <typename F>
bool PSCCalculator::addEcoEdgesAndCheckCond(CheckPSCType t,
					     const std::vector<Event> &fcs,
					     Matrix2D<Event> &matrix, F cond) const
{
	const auto *cohTracker = getCoherenceCalculator();

	std::vector<const llvm::GenericValue *> scLocs = getDoubleLocs();
	if (auto *moTracker = llvm::dyn_cast<MOCoherenceCalculator>(cohTracker)) {
		switch (t) {
		case CheckPSCType::nocheck:
			return true;
		case CheckPSCType::weak:
		case CheckPSCType::wb:
			WARN_ONCE("check-mo-psc", "The full PSC condition is going "
				  "to be checked for the MO-tracking exploration...\n");
		case CheckPSCType::full:
			return addSCEcosMO(fcs, scLocs, matrix, cond);
		default:
			WARN("Unimplemented model!\n");
			BUG();
		}
	} else if (auto *wbTacker = llvm::dyn_cast<WBCoherenceCalculator>(cohTracker)) {
		switch (t) {
		case CheckPSCType::nocheck:
			return true;
		case CheckPSCType::weak:
			return addSCEcosWBWeak(fcs, scLocs, matrix, cond);
		case CheckPSCType::wb:
			return addSCEcosWB(fcs, scLocs, matrix, cond);
		case CheckPSCType::full:
			return addSCEcosWBFull(fcs, scLocs, matrix, cond);
		default:
			WARN("Unimplemented model!\n");
			BUG();
		}
	}
	BUG();
	return false;
}


template <typename F>
bool PSCCalculator::checkPscCondition(CheckPSCType t, F cond) const
{
	/* Collect all SC events (except for RMW loads) */
	auto accesses = getSCs();
	auto &scs = accesses.first;
	auto &fcs = accesses.second;

	/* If there are no SC events, it is a valid execution */
	if (scs.empty())
		return true;

	/* Depending on the part of PSC calculated, instantiate the matrix */
	Matrix2D<Event> matrix(scs);

	/* Add edges from the initializer write (special case) */
	addInitEdges(fcs, matrix);
	/* Add sb and sb_(<>loc);hb;sb_(<>loc) edges (+ Fsc;hb;Fsc) */
	addSbHbEdges(matrix);

	/*
	 * Collect memory locations with more than one SC accesses
	 * and add the rest of PSC_base and PSC_fence
	 */
	return addEcoEdgesAndCheckCond(t, fcs, matrix, cond);
}

template bool PSCCalculator::checkPscCondition<bool (*)(const Matrix2D<Event>&)>
(CheckPSCType, bool (*)(const Matrix2D<Event>&)) const;
template bool PSCCalculator::checkPscCondition<std::function<bool(const Matrix2D<Event>&)>>
(CheckPSCType, std::function<bool(const Matrix2D<Event>&)>) const;

bool __isPscAcyclic(const Matrix2D<Event> &psc)
{
	return psc.isIrreflexive();
}

bool PSCCalculator::isPscAcyclic(CheckPSCType t) const
{
	return checkPscCondition(t, __isPscAcyclic);
}
