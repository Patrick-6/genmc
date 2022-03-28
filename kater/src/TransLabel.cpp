#include "TransLabel.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

int RelLabel::calcNum = 0;

/* Meaning of the bitmask:
	| other
	| sc, relacq, rel, acq, na
	| Faul, Fas, Faa, Fba, Frmb, Fwmb, F-other
	| UW, W\UW, UR, R\UR
	| RfInit, !RfInit
	| static-loc, dynamic-loc
	| hpProtected, !hpProtected */
static const unsigned other_event_bitmask =  0b1'00000'0000000'0000'00'00'00;

const std::vector<PredicateInfo> builtinPredicates = {
	/* Access modes */
	{"NA",             0b0'00001'0000000'1111'11'11'11, "#->isNotAtomic()"},
	{"ACQ",            0b0'11010'0000011'1111'11'11'11, "#->isAtLeastAcquire()"},
	{"REL",            0b0'11100'0000011'1111'11'11'11, "#->isAtLeastRelease()"},
	{"SC",             0b0'10000'0000011'1111'11'11'11, "#->isSC()"},
	/* Random stuff */
	{"IsDynamicLoc",   0b0'11111'0000000'1111'11'01'11, "g.is_dynamic_loc(#)"},
	{"NotHpProtected", 0b0'11111'0000000'1111'11'11'01, "g.notHpProtected(#)"},
	{"RfInit",         0b0'11111'0000000'0011'10'11'11, "llvm::isa<ReadLabel>(#) && llvm::dyn_cast<ReadLabel>(#)->getRf().isInitializer()"},
	/* Memory accesses */
	{"MemAccess",      0b0'11111'0000000'1111'11'11'11, "llvm::isa<MemAccessLabel>(#)"},
	{"W",              0b0'11111'0000000'1100'00'11'11, "llvm::isa<WriteLabel>(#)"},
	{"UW",             0b0'11111'0000000'1000'00'11'11, "g.isRMWStore(#)"},
	{"R",              0b0'11111'0000000'0011'11'11'11, "llvm::isa<ReadLabel>(#)"},
	{"UR",             0b0'11111'0000000'0010'11'11'11, "g.isRMWLoad(#)"},
//	{"HelpingCas",     0b0'11111'0000000'0000'11'11'11, "llvm::isa<HelpingCasLabel>(#)"},
	/* Fences */
        {"F",              0b0'11111'1111111'0000'00'00'00, "llvm::isa<FenceLabel>(#)"},
	{"Fwmb",           0b0'00000'0000010'0000'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::WMB)"},
	{"Frmb",           0b0'00000'0000100'0000'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::RMB)"},
	{"Fba",            0b0'00000'0001000'0000'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBBA)"},
	{"Faa",            0b0'00000'0010000'0000'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBAA)"},
	{"Fas",            0b0'00000'0100000'0000'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBAS)"},
	{"Faul",           0b0'00000'1000000'0000'00'00'00, "SmpFenceLabelLKMM::isType(#, SmpFenceType::MBAUL)"},
	/* Thread events */
	{"ThreadCreate",   other_event_bitmask, "llvm::isa<ThreadCreateLabel>(#)"},
	{"ThreadJoin",     other_event_bitmask, "llvm::isa<ThreadJoinLabel>(#)"},
	{"ThreadKill",     other_event_bitmask, "llvm::isa<ThreadKillLabel>(#)"},
	{"ThreadStart",    other_event_bitmask, "llvm::isa<ThreadStartLabel>(#)"},
	{"ThreadFinish",   other_event_bitmask, "llvm::isa<ThreadFinishLabel>(#)"},
	/* Allocation */
	{"Alloc",          other_event_bitmask, "llvm::isa<MallocLabel>(#)"},
	{"Free",           other_event_bitmask, "llvm::isa<FreeLabel>(#)"},
	{"HpRetire",       other_event_bitmask, "llvm::isa<HpRetireLabel>(#)"},
	{"HpProtect",      other_event_bitmask, "llvm::isa<HpProtectLabel>(#)"},
	/* Mutexes */
	{"Lock",           other_event_bitmask, "llvm::isa<LockLabel>(#)"},
	{"Unlock",         other_event_bitmask, "llvm::isa<UnlockLabel>(#)"},
	/* RCU */
	{"RCUSync",        other_event_bitmask, "llvm::isa<RCUSyncLabel>(#)"},
	{"RCULock",        other_event_bitmask, "llvm::isa<RCULockLabel>(#)"},
	{"RCUUnlock",      other_event_bitmask, "llvm::isa<RCUUnlockLabel>(#)"},
	/* File ops */
//	{"DskWrite",       other_event_bitmask, "llvm::isa<DskWriteLabel>(#)"},
//	{"DskMdWrite",     other_event_bitmask, "llvm::isa<DskMdWriteLabel>(#)"},
//	{"DskDirWrite",    other_event_bitmask, "llvm::isa<DskDirWriteLabel>(#)"},
//	{"DskJnlWrite",    other_event_bitmask, "llvm::isa<DskJnlWriteLabel>(#)"},
	{"DskOpen",        other_event_bitmask, "llvm::isa<DskOpenLabel>(#)"},
	{"DskFsync",       other_event_bitmask, "llvm::isa<DskFsyncLabel>(#)"},
	{"DskSync",        other_event_bitmask, "llvm::isa<DskSyncLabel>(#)"},
	{"DskPbarrier",    other_event_bitmask, "llvm::isa<DskPbarrierLabel>(#)"},
};

const std::vector<RelationInfo> builtinRelations = {
        /* program order */
        {"po-imm",      RelType::OneOne,     true,  "po_imm_succs",     "po_imm_preds"},
        {"po-loc-imm",  RelType::OneOne,     true,  "poloc_imm_succs",  "poloc_imm_preds"},
	/* intra-thread dependencies */
        {"ctrl-imm",    RelType::UnsuppMany, true,  "?",                "ctrl_preds"},
        {"addr-imm",    RelType::UnsuppMany, true,  "?",                "addr_preds"},
        {"data-imm",    RelType::UnsuppMany, true,  "?",                "data_preds"},
	/* same thread */
	{"same-thread", RelType::Conj,	     false, "same_thread",      "same_thread"},
	/* same location */
        {"alloc",       RelType::ManyOne,    false, "alloc_succs",      "alloc"},
        {"frees",       RelType::OneOne,     false, "frees",            "alloc"},
        {"loc-overlap", RelType::Final,      false, "?",                "loc_preds"},
	/* reads-from, coherence, from-read, detour */
        {"rf",          RelType::ManyOne,    false, "rf_succs",          "rf_preds"},
        {"rfe",         RelType::ManyOne,    false, "rfe_succs",         "rfe_preds"},
        {"rfi",         RelType::ManyOne,    false, "rfi_succs",         "rfi_preds"},
        {"mo-imm",      RelType::OneOne,     false, "co_succs",          "co_preds"},
        {"fr-init",     RelType::OneOne,     false, "fr_init_succs",     "fr_init_preds"},
        {"detour",      RelType::OneOne,     false, "detour_succs",      "detour_preds"},
};

static bool is_sub_predicate(int i, int j)
{
	auto bi = builtinPredicates[i].bitmask;
	auto bj = builtinPredicates[j].bitmask;
	return bi != other_event_bitmask && (bi & bj) == bi;
}

static void simplify_preds(std::set<int> &s)
{
	for (auto it = s.begin(); it != s.end(); /* */) {
		/* remove (*it) if there exists a more specific predicate in the set */
		if (std::any_of (s.begin(), s.end(), [&](auto i) {
				return i != *it && is_sub_predicate (i, *it); }))
			it = s.erase(it);
		else
			it++;
	}
}

std::string PredLabel::toString() const
{
	std::string s;
	bool not_first = false;
	for (auto &i : preds)
		s += (not_first ? ";" : (not_first = true, "["))
		     + builtinPredicates[i].name;
	return s + "]";
}

std::string RelLabel::toString() const
{
	std::string s =	isBuiltin() ?  builtinRelations[trans].name :
		std::string("$") + std::to_string(getCalcIndex());
	if (flipped)
		s += "^-1";
	return s;
}

bool PredLabel::merge (const PredLabel &other)
{
	unsigned mask = ~0;
	for (auto i : preds) mask &= builtinPredicates[i].bitmask;
	for (auto i : other.preds) mask &= builtinPredicates[i].bitmask;
	if (mask == 0 ||
	    (mask == other_event_bitmask && preds != other.preds))
		return false;
	for (auto i : other.preds) preds.insert(i);
	simplify_preds(preds);
	return true;
}
