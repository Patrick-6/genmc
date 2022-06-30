#include "Predicate.hpp"
#include <cassert>
#include <iostream>

/* Meaning of the bitmask:
	| other
	| sc, relacq, rel, acq, rlx, na
	| Faul, Fas, Faa, Fba, Frmb, Fwmb
	| F, UW, W\UW, UR, R\UR
	| RfInit, !RfInit
	| static-loc, dynamic-loc
	| hpProtected, !hpProtected
 	| REC, !REC
	| D, !D */
static const unsigned other_event_bitmask =  0b1'000000'000000'00000'00'00'00'00'00;
static const unsigned track_event_bitmask =  0b0'000000'000000'11111'00'00'00'00'00;
static const unsigned access_mode_bitmask =  0b0'111111'111111'00000'00'00'00'00'00;

const std::unordered_map<Predicate::Builtin, PredicateInfo> Predicate::builtins = {
	/* Access modes */
	{Marked,	{"Marked",         0b0'111110'000000'01111'11'11'11'11'11}},
	{NA,		{"NA",             0b0'000001'000000'00101'11'11'11'11'11}},
	{ACQ,		{"ACQ",            0b0'110100'000000'10011'11'11'11'11'11}},
	{REL,		{"REL",            0b0'111000'000000'11100'11'11'11'11'11}},
	{SC,		{"SC",             0b0'100000'000000'11111'11'11'11'11'11}},
	/* Random stuff */
	{IsDynamicLoc,	{"IsDynamicLoc",   0b0'111111'000000'01111'11'01'11'11'11}},
	{NotHpProtected,{"NotHpProtected", 0b0'111111'000000'01111'11'11'01'11'11}},
	{RfInit,	{"RfInit",         0b0'111111'000000'00011'10'11'11'11'11}},
	{REC,		{"REC",            0b0'111111'000000'00011'11'11'11'10'11}},
	{D,		{"D",              0b0'111111'000000'01111'11'11'11'11'10}},
	/* Memory accesses */
	{MemAccess,	{"MemAccess",      0b0'111111'000000'01111'11'11'11'11'11}},
	{W,		{"W",              0b0'111111'000000'01100'00'11'11'01'11}},
	{UW,		{"UW",             0b0'111111'000000'01000'00'11'11'01'11}},
	{R,		{"R",              0b0'111111'000000'00011'11'11'11'11'11}},
	{UR,		{"UR",             0b0'111111'000000'00010'11'11'11'11'11}},
//	{HelpingCas,	{"HelpingCas",     0b0'111111'000000'00000'11'11'11'11'11}},
	/* Fences */
        {F,		{"F",              0b0'111110'000000'10000'00'00'00'00'00}},
	{Fwmb,		{"Fwmb",           0b0'000000'000001'10000'00'00'00'00'00}},
	{Frmb,		{"Frmb",           0b0'000000'000010'10000'00'00'00'00'00}},
	{Fba,		{"Fba",            0b0'000000'000100'10000'00'00'00'00'00}},
	{Faa,		{"Faa",            0b0'000000'001000'10000'00'00'00'00'00}},
	{Fas,		{"Fas",            0b0'000000'010000'10000'00'00'00'00'00}},
	{Faul,		{"Faul",           0b0'000000'100000'10000'00'00'00'00'00}},
	/* Thread events */
	{TC,		{"TC",             other_event_bitmask}},
	{TJ,		{"TJ",             other_event_bitmask}},
	{TK,		{"TK",             other_event_bitmask}},
	{TB,		{"TB",             other_event_bitmask}},
	{TE,		{"TE",             other_event_bitmask}},
	/* Allocation */
	{Alloc,		{"Alloc",          other_event_bitmask}},
	{Free,		{"Free",           other_event_bitmask}},
	{HpRetire,	{"HpRetire",       other_event_bitmask}},
	{HpProtect,	{"HpProtect",      other_event_bitmask}},
	/* Mutexes */
	{LR,		{"LR",             other_event_bitmask}},
	{LW,		{"LW",             other_event_bitmask}},
	{UL,		{"UL",             other_event_bitmask}},
	/* RCU */
	{RCUSync,	{"RCUSync",        other_event_bitmask}},
	{RCULock,	{"RCULock",        other_event_bitmask}},
	{RCUUnlock,	{"RCUUnlock",      other_event_bitmask}},
	/* File ops */
//	{DskWrite,	{"DskWrite",       other_event_bitmask}},
//	{DskMdWrite,	{"DskMdWrite",     other_event_bitmask}},
//	{DskDirWrite,	{"DskDirWrite",    other_event_bitmask}},
//	{DskJnlWrite,	{"DskJnlWrite",    other_event_bitmask}},
	{DskOpen,	{"DskOpen",        other_event_bitmask}},
	{DskFsync,	{"DskFsync",       other_event_bitmask}},
	{DskSync,	{"DskSync",        other_event_bitmask}},
	{DskPbarrier,	{"DskPbarrier",    other_event_bitmask}},
	{CLF,		{"CLF",            other_event_bitmask}},
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
	if (empty())
		return true;
	if (other.empty())
		return false;

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

	unsigned mask = ~0;
	std::for_each(begin(), end(), [&mask](auto &p){
		assert(p.isBuiltin());
		mask &= Predicate::builtins.find(p.toBuiltin())->second.bitmask;
	});
	std::for_each(other.begin(), other.end(), [&mask](auto &p){
		assert(p.isBuiltin());
		mask &= Predicate::builtins.find(p.toBuiltin())->second.bitmask;
	});
	return (mask & (track_event_bitmask | other_event_bitmask)) != 0 &&
		(mask & (access_mode_bitmask | other_event_bitmask)) != 0 &&
		(mask != other_event_bitmask || *this == other);
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

void PredicateSet::minus(const PredicateSet &other)
{
	std::for_each(other.begin(), other.end(), [this](auto &p){
		preds_.erase(p);
	});
	return;
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
