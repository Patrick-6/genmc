#include "Predicate.hpp"
#include <cassert>
#include <iostream>

/* Meaning of the bitmask:
	| other
	| sc, relacq, rel, acq, na
	| Faul, Fas, Faa, Fba, Frmb, Fwmb, F-other
	| UW, W\UW, UR, R\UR
	| RfInit, !RfInit
	| static-loc, dynamic-loc
	| hpProtected, !hpProtected
 	| REC, !REC
	| D, !D */
static const unsigned other_event_bitmask =  0b1'00000'0000000'0000'00'00'00'00'00;
static const unsigned track_event_bitmask =  0b0'00000'1111111'1111'00'00'00'00'00;

const std::unordered_map<Predicate::Builtin, PredicateInfo> Predicate::builtins = {
	/* Access modes */
	{Marked,	{"Marked",         0b0'11110'0000000'1111'11'11'11'11'11, "!#->isNotAtomic()"}},
	{NA,		{"NA",             0b0'00001'0000000'1111'11'11'11'11'11, "#->isNotAtomic()"}},
	{ACQ,		{"ACQ",            0b0'11010'0000011'1111'11'11'11'11'11, "#->isAtLeastAcquire()"}},
	{REL,		{"REL",            0b0'11100'0000011'1111'11'11'11'11'11, "#->isAtLeastRelease()"}},
	{SC,		{"SC",             0b0'10000'0000011'1111'11'11'11'11'11, "#->isSC()"}},
	/* Random stuff */
	{IsDynamicLoc,	{"IsDynamicLoc",   0b0'11111'0000000'1111'11'01'11'11'11, "g.is_dynamic_loc(#)"}},
	{NotHpProtected,{"NotHpProtected", 0b0'11111'0000000'1111'11'11'01'11'11, "g.notHpProtected(#)"}},
	{RfInit,	{"RfInit",         0b0'11111'0000000'0011'10'11'11'11'11, "llvm::isa<ReadLabel>(#) && llvm::dyn_cast<ReadLabel>(#)->getRf().isInitializer()"}},
	{REC,		{"REC",            0b0'11111'0000000'0011'11'11'11'10'11, "#->getThread() == g.getRecoveryRoutineId()"}},
	{D,		{"D",              0b0'11111'0000000'1111'11'11'11'11'10, "llvm::isa<MemAccessLabel>(#) && llvm::dyn_cast<MemAccessLabel>(#)->getAddr().isDurable()"}},
	/* Memory accesses */
	{MemAccess,	{"MemAccess",      0b0'11111'0000000'1111'11'11'11'11'11, "llvm::isa<MemAccessLabel>(#)"}},
	{W,		{"W",              0b0'11111'0000000'1100'00'11'11'01'11, "llvm::isa<WriteLabel>(#)"}},
	{UW,		{"UW",             0b0'11111'0000000'1000'00'11'11'01'11, "g.isRMWStore(#)"}},
	{R,		{"R",              0b0'11111'0000000'0011'11'11'11'11'11, "llvm::isa<ReadLabel>(#)"}},
	{UR,		{"UR",             0b0'11111'0000000'0010'11'11'11'11'11, "g.isRMWLoad(#)"}},
//	{HelpingCas,	{"HelpingCas",     0b0'11111'0000000'0000'11'11'11'11'11, "llvm::isa<HelpingCasLabel>(#)"}},
	/* Fences */
        {F,		{"F",              0b0'11111'1111111'0000'00'00'00'00'00, "llvm::isa<FenceLabel>(#)"}},
	{Fwmb,		{"Fwmb",           0b0'00000'0000010'0000'00'00'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::WMB)"}},
	{Frmb,		{"Frmb",           0b0'00000'0000100'0000'00'00'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::RMB)"}},
	{Fba,		{"Fba",            0b0'00000'0001000'0000'00'00'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBBA)"}},
	{Faa,		{"Faa",            0b0'00000'0010000'0000'00'00'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBAA)"}},
	{Fas,		{"Fas",            0b0'00000'0100000'0000'00'00'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBAS)"}},
	{Faul,		{"Faul",           0b0'00000'1000000'0000'00'00'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBAUL)"}},
	/* Thread events */
	{TC,		{"TC",             other_event_bitmask, "llvm::isa<ThreadCreateLabel>(#)"}},
	{TJ,		{"TJ",             other_event_bitmask, "llvm::isa<ThreadJoinLabel>(#)"}},
	{TK,		{"TK",             other_event_bitmask, "llvm::isa<ThreadKillLabel>(#)"}},
	{TB,		{"TB",             other_event_bitmask, "llvm::isa<ThreadStartLabel>(#)"}},
	{TE,		{"TE",             other_event_bitmask, "llvm::isa<ThreadFinishLabel>(#)"}},
	/* Allocation */
	{Alloc,		{"Alloc",          other_event_bitmask, "llvm::isa<MallocLabel>(#)"}},
	{Free,		{"Free",           other_event_bitmask, "llvm::isa<FreeLabel>(#)"}},
	{HpRetire,	{"HpRetire",       other_event_bitmask, "llvm::isa<HpRetireLabel>(#)"}},
	{HpProtect,	{"HpProtect",      other_event_bitmask, "llvm::isa<HpProtectLabel>(#)"}},
	/* Mutexes */
	{LR,		{"LR",             other_event_bitmask, "llvm::isa<LockCasReadLabel>(#)"}},
	{LW,		{"LW",             other_event_bitmask, "llvm::isa<LockCasWriteLabel>(#)"}},
	{UL,		{"UL",             other_event_bitmask, "llvm::isa<UnlockWriteLabel>(#)"}},
	/* RCU */
	{RCUSync,	{"RCUSync",        other_event_bitmask, "llvm::isa<RCUSyncLabel>(#)"}},
	{RCULock,	{"RCULock",        other_event_bitmask, "llvm::isa<RCULockLabel>(#)"}},
	{RCUUnlock,	{"RCUUnlock",      other_event_bitmask, "llvm::isa<RCUUnlockLabel>(#)"}},
	/* File ops */
//	{DskWrite,	{"DskWrite",       other_event_bitmask, "llvm::isa<DskWriteLabel>(#)"}},
//	{DskMdWrite,	{"DskMdWrite",     other_event_bitmask, "llvm::isa<DskMdWriteLabel>(#)"}},
//	{DskDirWrite,	{"DskDirWrite",    other_event_bitmask, "llvm::isa<DskDirWriteLabel>(#)"}},
//	{DskJnlWrite,	{"DskJnlWrite",    other_event_bitmask, "llvm::isa<DskJnlWriteLabel>(#)"}},
	{DskOpen,	{"DskOpen",        other_event_bitmask, "llvm::isa<DskOpenLabel>(#)"}},
	{DskFsync,	{"DskFsync",       other_event_bitmask, "llvm::isa<DskFsyncLabel>(#)"}},
	{DskSync,	{"DskSync",        other_event_bitmask, "llvm::isa<DskSyncLabel>(#)"}},
	{DskPbarrier,	{"DskPbarrier",    other_event_bitmask, "llvm::isa<DskPbarrierLabel>(#)"}},
	{CLF,		{"CLF",            other_event_bitmask, "llvm::isa<CLFlushLabel>(#)"}},
};

bool Predicate::subOf(const Predicate &other) const
{
	assert(isBuiltin() && other.isBuiltin());
	auto bi = Predicate::builtins.find(toBuiltin())->second.bitmask;
	auto bj = Predicate::builtins.find(other.toBuiltin())->second.bitmask;
	return bi != other_event_bitmask && (bi & bj) == bi;
}

std::ostream &operator<<(std::ostream& ostr, const Predicate &p)
{
	assert(p.isBuiltin());
	ostr << Predicate::builtins.find(p.toBuiltin())->second.name;
	return ostr;
}


bool PredicateSet::includes(const PredicateSet &other) const
{
       unsigned mask1 = ~0;
       std::for_each(begin(), end(), [&mask1](auto &p){
		mask1 &= Predicate::builtins.find(p.toBuiltin())->second.bitmask;
       });
       unsigned mask2 = ~0;
       std::for_each(other.begin(), other.end(), [&mask2](auto &p){
		mask2 &= Predicate::builtins.find(p.toBuiltin())->second.bitmask;
       });
       return (mask1 | mask2) == mask1;
}

bool PredicateSet::composes(const PredicateSet &other) const
{
	if (empty() || other.empty())
		return true; /* class invariant */

	unsigned mask = track_event_bitmask | other_event_bitmask;
	std::for_each(begin(), end(), [&mask](auto &p){
		assert(p.isBuiltin());
		mask &= Predicate::builtins.find(p.toBuiltin())->second.bitmask;
	});
	std::for_each(other.begin(), other.end(), [&mask](auto &p){
		assert(p.isBuiltin());
		mask &= Predicate::builtins.find(p.toBuiltin())->second.bitmask;
	});
	return mask != 0 && (mask != other_event_bitmask || *this == other);
}

void PredicateSet::simplify()
{
	/* No erase_if for sets and the like ... */
	for (auto it = preds_.begin(); it != preds_.end(); /* */) {
		if (std::any_of(preds_.begin(), preds_.end(), [&](auto &p2) {
				return *it != p2 && p2.subOf(*it);
		}))
			it = preds_.erase(it);
		else
			it++;
	}
}

bool PredicateSet::merge(const PredicateSet &other)
{
	if (!composes(other))
		return false;

	preds_.insert(other.begin(), other.end());
	simplify();
	return true;
}

std::ostream &operator<<(std::ostream& ostr, const PredicateSet &s)
{
	ostr << "[";

	auto first = true;
	for (auto &p : s) {
		ostr << (!first ? "&" : "") << p;
		first = false;
	}

	ostr << "]";
	return ostr;
}
