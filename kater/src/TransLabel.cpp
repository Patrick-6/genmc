#include "TransLabel.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

int TransLabel::calcNum = 0;

const std::vector<PredicateInfo> builtinPredicates = {
	/* Memory accesses */
	{"MemAccess",      "llvm::isa<MemAccessLabel>(#)"},
	{"W",              "llvm::isa<WriteLabel>(#)"},
	{"UW",             "G.isRMWStore(#)"},
	{"R",              "llvm::isa<ReadLabel>(#)"},
	{"UR",             "G.isRMWLoad(#)"},
//	{"HelpingCas",     "llvm::isa<HelpingCasLabel>(#)"},
	/* Fences */
        {"F",              "llvm::isa<FenceLabel>(#)"},
//	{"SmpFence",       "llvm::isa<SmpFenceLabel>(#)"},
	/* Thread events */
	{"ThreadCreate",   "llvm::isa<ThreadCreateLabel>(#)"},
	{"ThreadJoin",     "llvm::isa<ThreadJoinLabel>(#)"},
	{"ThreadKill",     "llvm::isa<ThreadKillLabel>(#)"},
	{"ThreadStart",    "llvm::isa<ThreadStartLabel>(#)"},
	{"ThreadFinish",   "llvm::isa<ThreadFinishLabel>(#)"},
	/* Allocation & deallocation */
	{"Alloc",          "llvm::isa<MallocLabel>(#)"},
	{"Free",           "llvm::isa<FreeLabel>(#)"},
	{"HpRetire",       "llvm::isa<HpRetireLabel>(#)"},
	{"HpProtect",      "llvm::isa<HpProtectLabel>(#)"},
	/* Mutexes */
	{"Lock",           "llvm::isa<LockLabel>(#)"},
	{"Unlock",         "llvm::isa<UnlockLabel>(#)"},
	/* RCU */
	{"RCUSync",        "llvm::isa<RCUSyncLabel>(#)"},
	{"RCULock",        "llvm::isa<RCULockLabel>(#)"},
	{"RCUUnlock",      "llvm::isa<RCUUnlockLabel>(#)"},
	/* File system operations */
//	{"DskWrite",       "llvm::isa<DskWriteLabel>(#)"},
//	{"DskMdWrite",     "llvm::isa<DskMdWriteLabel>(#)"},
//	{"DskDirWrite",    "llvm::isa<DskDirWriteLabel>(#)"},
//	{"DskJnlWrite",    "llvm::isa<DskJnlWriteLabel>(#)"},
	{"DskOpen",        "llvm::isa<DskOpenLabel>(#)"},
	{"DskFsync",       "llvm::isa<DskFsyncLabel>(#)"},
	{"DskSync",        "llvm::isa<DskSyncLabel>(#)"},
	{"DskPbarrier",    "llvm::isa<DskPbarrierLabel>(#)"},
	/* Access modes */
	{"NA",             "#.isNotAtomic()"},
//	{"RLX",            "(#.getOrdering() == llvm::AtomicOrdering::Monotonic)"},
	{"ACQ",            "#.isAtLeastAcquire()"},
	{"REL",            "#.isAtLeastRelease()"},
	{"SC",             "#.isSC()"},
	/* Random stuff */
	{"IsDynamicLoc",   "G.is_dynamic_loc(#)"},
	{"NotHpProtected", "G.notHpProtected(#)"},
	/* Initializer */
	{"INIT",           "#.isInitializer()"}};

const std::vector<RelationInfo> builtinRelations = {
        /* program order */
        {"po-imm",      RelType::OneOne,     "po_succ",      "po_pred"},
        {"po-loc",      RelType::OneOne,     "po_loc_succ",  "po_loc_pred"},
	/* intra-thread dependencies */
        {"ctrl-imm",    RelType::UnsuppMany, "?",            "ctrl_preds"},
        {"addr-imm",    RelType::UnsuppMany, "?",            "addr_preds"},
        {"data-imm",    RelType::UnsuppMany, "?",            "data_preds"},
	/* same thread */
	{"same-thread", RelType::Conj,	     "same_thread",  "same_thread"},
	/* same location */
        {"alloc",       RelType::ManyOne,    "alloc_succs",  "alloc"},
        {"frees",       RelType::OneOne,     "frees",        "alloc"},
        {"loc-overlap", RelType::Final,      "?",            "loc_preds"},
	/* reads-from, coherence, from-read */
        {"rf",          RelType::ManyOne,    "rf_succs",     "rf_pred"},
        {"rfe",         RelType::ManyOne,    "rfe_succs",    "rfe_pred"},
        {"mo-imm",      RelType::OneOne,     "mo_succ",      "mo_pred"},
        {"fr-init",     RelType::OneOne,     "fr_init_succ", "fr_init_pred"}};

std::ostream &operator<<(std::ostream &s, const TransLabel &t)
{
	if (t.isPredicate())
		return s << builtinPredicates[t.trans].name;
	if (t.isBuiltin())
		s << builtinRelations[t.trans - builtinPredicates.size()].name;
	else
		s << "$" <<  (t.trans - builtinPredicates.size() - builtinRelations.size());
	if (t.flipped)
		s << "^-1";
	return s;
}

void TransLabel::output_for_genmc (std::ostream& ostr,
				  const std::string &arg,
				  const std::string &res) const
{
	if (isPredicate()) {
		auto s = builtinPredicates[trans].genmcString;
		s.replace(s.find_first_of('#'), 1, arg);
		ostr << "\tif (" << s << ") {\n";
		ostr << "\t\tauto " << res << " = " << arg << ";\n";
		return;
	}
	if (isBuiltin()) {
		const auto &n = builtinRelations[trans - builtinPredicates.size()];
		const auto &s = flipped ? n.predString : n.succString;
		if ((n.type == RelType::OneOne) || (flipped && n.type == RelType::ManyOne))
			ostr << "\tif (auto " << res << " = " << s << ") {\n";
		else
			ostr << "\tfor (auto &" << res << " : " << s << ") {\n";
		return;
	}
	int k = trans - builtinPredicates.size() - builtinRelations.size();
	ostr << "\tfor (auto &" << res << " : calculator" << k << "(" << arg << "))\n";
}

//static std::vector<std::set<std::string> > invalids = {};
//static std::vector<TransLabel> invalids2 = {};
//
//void TransLabel::register_invalid (const TransLabel &l)
//{
//	// Do not register already-invalid equations
//	if (!l.is_valid()) return;
//
//	if (l.is_empty_trans()) {
//		if (std::find (invalids.begin(), invalids.end(), l.pre_checks) == invalids.end())
//			invalids.push_back(l.pre_checks);
//	} else {
//		if (std::find (invalids2.begin(), invalids2.end(), l) == invalids2.end())
//			invalids2.push_back(l);
//	}
//}
//
//void TransLabel::make_bracket ()
//{
//	assert (pre_checks.empty() && post_checks.empty());
//	pre_checks.insert (trans);
//	trans.clear();
//}
//
//TransLabel::my_set TransLabel::merge_sets (const TransLabel::my_set &a,
//					   const TransLabel::my_set &b)
//{
//	/* `r = a \cup b` */
//	auto r = a;
//	for (const auto &e : b) r.insert(e);
//
//	/* `if (\exists s \in invalids. s \subseteq r) r = \emptyset` */
//	auto is_in_r = [&](const std::string &s) {
//		return r.find(s) != r.end();
//	};
//	auto subset_of_r = [&](const TransLabel::my_set &s) {
//		return std::all_of (s.begin(), s.end(), is_in_r);
//	};
//	if (std::any_of (invalids.begin(), invalids.end(), subset_of_r))
//		r.clear();
//	return r;
//}
//
//void TransLabel::flip ()
//{
//	if (is_empty_trans()) return;
//	std::swap (pre_checks, post_checks);
//}
//
//TransLabel TransLabel::seq (const TransLabel &other) const
//{
//	TransLabel r;
//	if (!is_valid() || !other.is_valid())
//		return r;
//	if (is_empty_trans()) {
//		r.pre_checks = TransLabel::merge_sets (pre_checks, other.pre_checks);
//		if (!r.pre_checks.empty()) { /* is valid? */
//			r.trans = other.trans;
//			r.post_checks = other.post_checks;
//		}
//	}
//	else {
//		assert (other.is_empty_trans());
//		r.post_checks = TransLabel::merge_sets (post_checks, other.pre_checks);
//		if (!r.post_checks.empty()) { /* is valid? */
//			r.trans = trans;
//			r.pre_checks = pre_checks;
//		}
//	}
//	if (!r.is_empty_trans()) {
//		for (const auto &i : invalids2) {
//			if (i.trans != r.trans) continue;
//			if (std::any_of(i.pre_checks.begin(), i.pre_checks.end(),
//					[&](const std::string &s) {
//					return r.pre_checks.find(s) == r.pre_checks.end();
//					}))
//				continue;
//			if (std::any_of(i.post_checks.begin(), i.post_checks.end(),
//					[&](const std::string &s) {
//					return r.post_checks.find(s) == r.post_checks.end();
//					}))
//				continue;
//			r.pre_checks.clear();
//			r.trans.clear();
//			r.post_checks.clear();
//			return r;
//		}
//	}
//	return r;
//}
