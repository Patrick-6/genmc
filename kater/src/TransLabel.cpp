#include "TransLabel.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

int RelLabel::calcNum = 0;

/* meaning of the bitmask:
        | sc, relacq, rel, acq, na
        | other, Fwmb, F\Fwmb, UW, W\UW, UR, R\UR
        | static-loc, dynamic-loc, hpProtected, !hpProtected */

const std::vector<PredicateInfo> builtinPredicates = {
	/* Memory accesses */
	{"MemAccess",      0b11111'0001111'1111, "llvm::isa<MemAccessLabel>(#)"},
	{"W",              0b11111'0001100'1111, "llvm::isa<WriteLabel>(#)"},
	{"UW",             0b11111'0001000'1111, "g.isRMWStore(#)"},
	{"R",              0b11111'0000011'1111, "llvm::isa<ReadLabel>(#)"},
	{"UR",             0b11111'0000010'1111, "g.isRMWLoad(#)"},
//	{"HelpingCas",     0b11111'0000000'1111, "llvm::isa<HelpingCasLabel>(#)"},
	/* Fences */
        {"F",              0b11111'0110000'0000, "llvm::isa<FenceLabel>(#)"},
//	{"Fwmb",           0b11111'0100000'0000, "llvm::isa<SmpFenceLabel>(#)"},
	/* Thread events */
	{"ThreadCreate",   0b00000'1000000'0000, "llvm::isa<ThreadCreateLabel>(#)"},
	{"ThreadJoin",     0b00000'1000000'0000, "llvm::isa<ThreadJoinLabel>(#)"},
	{"ThreadKill",     0b00000'1000000'0000, "llvm::isa<ThreadKillLabel>(#)"},
	{"ThreadStart",    0b00000'1000000'0000, "llvm::isa<ThreadStartLabel>(#)"},
	{"ThreadFinish",   0b00000'1000000'0000, "llvm::isa<ThreadFinishLabel>(#)"},
	/* Allocation */
	{"Alloc",          0b00000'1000000'0000, "llvm::isa<MallocLabel>(#)"},
	{"Free",           0b00000'1000000'0000, "llvm::isa<FreeLabel>(#)"},
	{"HpRetire",       0b00000'1000000'0000, "llvm::isa<HpRetireLabel>(#)"},
	{"HpProtect",      0b00000'1000000'0000, "llvm::isa<HpProtectLabel>(#)"},
	/* Mutexes */
	{"Lock",           0b00000'1000000'0000, "llvm::isa<LockLabel>(#)"},
	{"Unlock",         0b00000'1000000'0000, "llvm::isa<UnlockLabel>(#)"},
	/* RCU */
	{"RCUSync",        0b00000'1000000'0000, "llvm::isa<RCUSyncLabel>(#)"},
	{"RCULock",        0b00000'1000000'0000, "llvm::isa<RCULockLabel>(#)"},
	{"RCUUnlock",      0b00000'1000000'0000, "llvm::isa<RCUUnlockLabel>(#)"},
	/* File ops */
//	{"DskWrite",       0b11111'1000000'0000, "llvm::isa<DskWriteLabel>(#)"},
//	{"DskMdWrite",     0b11111'1000000'0000, "llvm::isa<DskMdWriteLabel>(#)"},
//	{"DskDirWrite",    0b11111'1000000'0000, "llvm::isa<DskDirWriteLabel>(#)"},
//	{"DskJnlWrite",    0b11111'1000000'0000, "llvm::isa<DskJnlWriteLabel>(#)"},
	{"DskOpen",        0b00000'1000000'0000, "llvm::isa<DskOpenLabel>(#)"},
	{"DskFsync",       0b00000'1000000'0000, "llvm::isa<DskFsyncLabel>(#)"},
	{"DskSync",        0b00000'1000000'0000, "llvm::isa<DskSyncLabel>(#)"},
	{"DskPbarrier",    0b00000'1000000'0000, "llvm::isa<DskPbarrierLabel>(#)"},
	/* Access modes */
	{"NA",             0b00001'0001111'1111, "#.isNotAtomic()"},
	{"ACQ",            0b11010'0111111'1111, "#.isAtLeastAcquire()"},
	{"REL",            0b11100'0111111'1111, "#.isAtLeastRelease()"},
	{"SC",             0b10000'0111111'1111, "#.isSC()"},
	/* Random stuff */
	{"IsDynamicLoc",   0b11111'0001111'0111, "g.is_dynamic_loc(#)"},
	{"NotHpProtected", 0b11111'0001111'1101, "g.notHpProtected(#)"},
	/* Initializer */
	{"INIT",           0b00001'0000100'0000, "#.isInitializer()"}};

const std::vector<RelationInfo> builtinRelations = {
        /* program order */
        {"po-imm",      RelType::OneOne,     true,  "po_succ",      "po_pred"},
        {"po-loc",      RelType::OneOne,     true,  "po_loc_succ",  "po_loc_pred"},
	/* intra-thread dependencies */
        {"ctrl-imm",    RelType::UnsuppMany, true,  "?",            "ctrl_preds"},
        {"addr-imm",    RelType::UnsuppMany, true,  "?",            "addr_preds"},
        {"data-imm",    RelType::UnsuppMany, true,  "?",            "data_preds"},
	/* same thread */
	{"same-thread", RelType::Conj,	     false, "same_thread",  "same_thread"},
	/* same location */
        {"alloc",       RelType::ManyOne,    false, "alloc_succs",  "alloc"},
        {"frees",       RelType::OneOne,     false, "frees",        "alloc"},
        {"loc-overlap", RelType::Final,      false, "?",            "loc_preds"},
	/* reads-from, coherence, from-read */
        {"rf",          RelType::ManyOne,    false, "rf_succs",     "rf_pred"},
        {"rfe",         RelType::ManyOne,    false, "rfe_succs",    "rfe_pred"},
        {"mo-imm",      RelType::OneOne,     false, "mo_succ",      "mo_pred"},
        {"fr-init",     RelType::OneOne,     false, "fr_init_succ", "fr_init_pred"}};


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

void PredLabel::output_for_genmc (std::ostream& ostr,
				  const std::string &arg,
				  const std::string &res) const
{
	bool not_first = false;
	if (preds.empty())
		ostr << "\t{\n";
	else {
		for (auto &i : preds) {
			if (not_first) {
				ostr << "\n\t   && ";
			} else {
				ostr << "\tif (";
				not_first = true;
			}
			auto s = builtinPredicates[i].genmcString;
			s.replace(s.find_first_of('#'), 1, arg);
			ostr << s;
		}
		ostr << ") {\n";
	}
	ostr << "\t\tauto " << res << " = " << arg << ";\n";
}

void RelLabel::output_for_genmc (std::ostream& ostr,
				  const std::string &arg,
				  const std::string &res) const
{
	if (isBuiltin()) {
		const auto &n = builtinRelations[trans];
		const auto &s = flipped ? n.predString : n.succString;
		if ((n.type == RelType::OneOne) || (flipped && n.type == RelType::ManyOne))
			ostr << "\tif (auto " << res << " = " << s << ") {\n";
		else
			ostr << "\tfor (auto &" << res << " : " << s << ") {\n";
		return;
	}
	ostr << "\tfor (auto &" << res << " : calculator" << getCalcIndex() << "(" << arg << "))\n";
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
