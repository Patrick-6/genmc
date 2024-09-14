#include "ExecutionGraph/GraphUtils.hpp"
#include "ExecutionGraph/ExecutionGraph.hpp"

bool isHazptrProtected(const MemAccessLabel *mLab)
{
	auto &g = *mLab->getParent();
	BUG_ON(!mLab->getAddr().isDynamic());

	auto *aLab = mLab->getAlloc();
	BUG_ON(!aLab);
	auto *pLab = llvm::dyn_cast_or_null<HpProtectLabel>(
		g.getPreviousLabelST(mLab, [&](const EventLabel *lab) {
			auto *pLab = llvm::dyn_cast<HpProtectLabel>(lab);
			return pLab && aLab->contains(pLab->getProtectedAddr());
		}));
	if (!pLab)
		return false;

	for (auto j = pLab->getIndex() + 1; j < mLab->getIndex(); j++) {
		auto *lab = g.getEventLabel(Event(mLab->getThread(), j));
		if (auto *oLab = llvm::dyn_cast<HpProtectLabel>(lab))
			if (oLab->getHpAddr() == pLab->getHpAddr())
				return false;
	}
	return true;
}
