/*
 * KATER -- Automating Weak Memory Model Metatheory
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
 */

#include "Config.hpp"
#include "Error.hpp"
#include "NFA.hpp"
#include <fstream>
#include <iostream>
#include <deque>

#define DEBUG_TYPE "nfa"

NFA::NFA(const TransLabel &c) : NFA()
{
	auto *init = createStarting();
	auto *fnal = createAccepting();
	addTransition(init, Transition(c, fnal));
	return;
}

static bool is_subset (const std::vector<char> &a, const std::vector<char> &b)
{
	for (int k = 0; k < a.size(); ++k)
		if (a[k] && !b[k]) return false;
	return true;
}

static void take_union (std::vector<char> &a, const std::vector<char> &b)
{
	for (int k = 0; k < a.size(); ++k)
		a[k] |= b[k];
}

static std::ostream & operator<< (std::ostream& ostr, const std::vector<char> &s)
{
	for (int c : s) ostr << (c ? "1" : ".");
	return ostr;
}

NFA &NFA::flip()
{
	std::for_each(states_begin(), states_end(), [](auto &s){
		s->flip();
	});
	std::swap(getStarting(), getAccepting());
	return *this;
}

// Basic graph clean up
void NFA::simplify_basic()
{
	// XXX: Candidate for rewrite
	bool changed;
	do {
		changed = false;

		for (auto it = states_begin(); it != states_end(); /* */ ) {
			if (((*it)->hasAllOutLoops() && !isAccepting(it->get())) ||
			    ((*it)->hasAllInLoops() && !isStarting(it->get()))) {
				it = removeState(it);
				changed = true;
			} else {
				++it;
			}
		}

		for (auto it = states_begin(); it != states_end(); ++it) {
			auto oit(it);
			++oit;
			while (oit != states_end()) {
				if (isAccepting(it->get()) == isAccepting(oit->get()) &&
				    (*it)->outgoingSameAs(oit->get())) {
					if (isStarting(oit->get()))
						makeStarting(it->get());
					addInvertedTransitions(it->get(), (*oit)->in_begin(), (*oit)->in_end());
					oit = removeState(oit);
					changed = true;
				} else {
					++oit;
				}
			}
		}

		for (auto it = states_begin(); it != states_end(); ++it) {
			auto oit(it);
			++oit;
			while (oit != states_end()) {
				if (isStarting(it->get()) == isStarting(oit->get()) &&
				    (*it)->incomingSameAs(oit->get())) {
					if (isAccepting(oit->get()))
						makeAccepting(it->get());
					addTransitions(it->get(), (*oit)->out_begin(), (*oit)->out_end());
					oit = removeState(oit);
					changed = true;
				} else {
					++oit;
				}
			}
		}
	} while (changed);
}

//-------------------------------------------------------------------

bool NFA::acceptsEmptyString() const
{
	return std::any_of(start_begin(), start_end(), [this](auto &s){
		return isAccepting(s);
	});
}

bool NFA::acceptsNoString(Counterexample &cex) const
{
	std::unordered_set<State *> visited;
	std::vector<std::pair<State *, Counterexample>> workList;

	for (auto it = states_begin(); it != states_end(); it++) {
		if (!isStarting(it->get()))
			continue;
		visited.insert(it->get());
		workList.push_back({it->get(), Counterexample()});
	}
	while (!workList.empty()) {
		auto [s, c] = workList.back();
		workList.pop_back();
		if (isAccepting(s)) {
			cex = c;
			return false;
		}

		for (auto it = s->out_begin(); it != s->out_end(); it++) {
			if (visited.count(it->dest))
				continue;
			visited.insert(it->dest);

			auto nc(c);
			nc.extend(it->label);
			workList.push_back({it->dest, nc});
		}
	}
	return true;
}

template<typename T>
inline void hash_combine(std::size_t& seed, std::size_t v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

bool NFA::isSubLanguageOfDFA(const NFA &other, Counterexample &cex,
			     std::function<bool(const TransLabel &)> isValidTransition) const
{
	KATER_DEBUG(
		std::cout << "Checking inclusion between automata:" << std::endl;
		std::cout << *this << "and " << other << std::endl;
	);

	if (other.getNumStarting() == 0)
		return acceptsNoString(cex);

	struct SPair {
		State *s1;
		State *s2;

		bool operator==(const SPair &other) const {
			return s1 == other.s1 && s2 == other.s2;
		}
	};
	// XXX: FIXME
	struct SPairHasher {
		std::size_t operator()(SPair p) const {
			std::size_t hash = 0;
			hash_combine<unsigned>(hash, p.s1->getId());
			hash_combine<unsigned>(hash, p.s2->getId());
			return hash;
		}
	};

	struct SimState {
		State *s1;
		State *s2;
		Counterexample cex;
	};

	std::unordered_set<SPair, SPairHasher> visited;
	std::vector<SimState> workList;

	std::for_each(start_begin(), start_end(), [&](auto *s1){
		std::for_each(other.start_begin(), other.start_end(), [&](auto *s2){
			visited.insert({s1, s2});
			workList.push_back({s1, s2, Counterexample()});
		});
	});
	while (!workList.empty()) {
		auto [s1, s2, c] = workList[0];
		workList.erase(workList.begin());
		if (isAccepting(s1) && !other.isAccepting(s2)) {
			cex = c;
			return false;
		}

		for (auto it = s1->out_begin(); it != s1->out_end(); ++it) {
			auto nc(c);
			nc.extend(it->label);

			auto canTakeEdge = false;
			for (auto oit = s2->out_begin(); oit != s2->out_end(); ++oit) {
				if (it->label != oit->label)
					continue;

				canTakeEdge = true;
				if (visited.count({it->dest, oit->dest}))
					continue;
				visited.insert({it->dest, oit->dest});
				workList.push_back({it->dest, oit->dest, nc});
			}
			if (!canTakeEdge) {
				cex = nc;
				return false;
			}
		}
	}
	return true;
}

bool NFA::isDFASubLanguageOfNFA(const NFA &other, Counterexample &cex,
				std::function<bool(const TransLabel &)> isValidTransition) const
{
	KATER_DEBUG(
		std::cout << "Checking inclusion between automata:" << std::endl;
		std::cout << *this << "and " << other << std::endl;
	);

	if (other.getNumStarting() == 0)
		return acceptsNoString(cex);

	struct SPair {
		State *s1;
		std::set<State *> ss2;

		bool operator==(const SPair &other) const {
			return s1 == other.s1 && ss2 == other.ss2;
		}
	};

	// XXX: FIXME
	struct SPairHasher {
		std::size_t operator()(SPair p) const {
			std::size_t hash = 0;
			hash_combine<unsigned>(hash, p.s1->getId());
			std::for_each(p.ss2.begin(), p.ss2.end(), [&](auto *s2){
				hash_combine<unsigned>(hash, s2->getId());
			});
			return hash;
		}
	};

	struct SimState {
		SPair cur;
		Counterexample cex;
	};

	std::unordered_set<SPair, SPairHasher> visited;
	std::deque<SimState> workList;

	std::for_each(start_begin(), start_end(), [&](auto *s1){
		std::set<State *> ss(other.start_begin(), other.start_end());
		visited.insert({s1, ss});
		workList.push_back({{s1, ss}, Counterexample()});
	});
	while (!workList.empty()) {
		auto [sp, c] = workList.front();
		workList.pop_front();

		if (isAccepting(sp.s1) &&
			std::none_of(sp.ss2.begin(), sp.ss2.end(),
				     [&](auto *s2) { return other.isAccepting(s2); })) {
			c.setType(Counterexample::Type::ANA);
			cex = c;
			return false;
		}

		for (auto it = sp.s1->out_begin(); it != sp.s1->out_end(); ++it) {
			auto nc(c);
			nc.extend(it->label);

			SPair next({it->dest, {}});
			std::for_each(sp.ss2.begin(), sp.ss2.end(), [&](auto *s2) {
				for (auto oit = s2->out_begin(); oit != s2->out_end(); ++oit) {
					if (!oit->label.matches(it->label))
						continue;
					next.ss2.insert(oit->dest);
				}
			});
			if (next.ss2.empty()) {
				cex = nc;
				return false;
			}

			if (visited.count(next))
				continue;
			visited.insert(next);
			workList.push_back({next, nc});
		}
	}
	return true;
}

NFA &NFA::alt(NFA &&other)
{
	std::move(other.states_begin(), other.states_end(), std::back_inserter(nfa));
	getStarting().insert(getStarting().end(), other.getStarting().begin(), other.getStarting().end());
	getAccepting().insert(getAccepting().end(), other.getAccepting().begin(), other.getAccepting().end());
	return *this;
}

NFA &NFA::seq(NFA &&other)
{
	/* Add transitions `this->accepting --> other.starting.outgoing` */
	std::for_each(accept_begin(), accept_end(), [&](auto &a){
		std::for_each(other.start_begin(), other.start_end(), [&](auto &s){
			addTransitions(a, s->out_begin(), s->out_end());
		});
	});

	/* Clear accepting states if necessary */
	if (!other.acceptsEmptyString())
		clearAllAccepting();

	/* Clear starting states of `other` */
	other.clearAllStarting();

	/* Move the states of the `other` NFA into our NFA and append accepting states */
	std::move(other.states_begin(), other.states_end(), std::back_inserter(nfa));
	getAccepting().insert(getAccepting().end(), other.getAccepting().begin(), other.getAccepting().end());
	return *this;
}

NFA &NFA::plus()
{
	/* Add transitions `accepting --> starting` */
	std::for_each(accept_begin(), accept_end(), [&](auto &a){
		std::for_each(start_begin(), start_end(), [&](auto &s){
			addEpsilonTransitionSucc(a, s);
		});
	});
	return *this;
}

NFA &NFA::or_empty()
{
	// Does the NFA already accept the empty string?
	if (acceptsEmptyString())
		return *this;

	// Otherwise, find starting node with no incoming edges
	auto it = std::find_if(start_begin(), start_end(), [](auto &s){ return !s->hasIncoming(); });

	auto *s = (it != start_end()) ? *it : createStarting();
	makeAccepting(s);
	return *this;
}

NFA &NFA::star()
{
	std::vector<State *> exStarting;
	std::vector<State *> exAccepting;
	std::copy(start_begin(), start_end(), std::back_inserter(exStarting));
	std::copy(accept_begin(), accept_end(), std::back_inserter(exAccepting));

	clearAllStarting();
	clearAllAccepting();

	/* Create a state that will be the new starting/accepting; do
	 * not change its status yet so that addEpsilon does not add
	 * starting/accepting states */
	auto *i = createState();

	std::for_each(exStarting.begin(), exStarting.end(), [&](auto *es){
		addEpsilonTransitionSucc(i, es);
	});
	std::for_each(exAccepting.begin(), exAccepting.end(), [&](auto *ea){
		addEpsilonTransitionPred(ea, i);
	});

	makeStarting(i);
	makeAccepting(i);
	assert(getNumAccepting() == getNumStarting() && getNumStarting() == 1);
	return *this;
}

// Convert to a deterministic automaton using the subset construction
std::pair<NFA, std::map<NFA::State *, std::set<NFA::State *>>> NFA::to_DFA() const
{
	NFA dfa;
	std::map<std::set<State *>, State *> nfaToDfaMap; // m
	std::map<State *, std::set<State *>> dfaToNfaMap; // v

	auto *s = dfa.createStarting();
	auto ss = std::set<State *>(start_begin(), start_end());
	nfaToDfaMap.insert({ss, s});
	dfaToNfaMap.insert({s, ss});

	std::vector<std::set<State *>> worklist = {ss};
	while (!worklist.empty()) {
		auto sc = worklist.back();
		worklist.pop_back();

		// XXX: FIXME
		std::for_each(sc.begin(), sc.end(), [&](State *ns){
			std::for_each(ns->out_begin(), ns->out_end(), [&](const Transition &t){
				std::set<State *> next;
				std::for_each(sc.begin(), sc.end(), [&](State *ns2){
					std::for_each(ns2->out_begin(), ns2->out_end(), [&](const Transition &t2){
						if (t2.label == t.label)
							next.insert(t2.dest);
					});
				});
				auto it = nfaToDfaMap.find(next);
				State *ds = nullptr;
				if (it != nfaToDfaMap.end()) {
					ds = it->second;
				} else {
					ds = dfa.createState();
					nfaToDfaMap.insert({next, ds});
					dfaToNfaMap.insert({ds, next});
					worklist.push_back(std::move(next));
				}
				dfa.addTransition(nfaToDfaMap[sc], Transition(t.label, ds));
			});
		});
	}

	std::for_each(dfaToNfaMap.begin(), dfaToNfaMap.end(), [&](auto &kv){
		if (std::any_of(kv.second.begin(), kv.second.end(), [&](State *s){
			return isAccepting(s);
		}))
			dfa.makeAccepting(kv.first);
	});
	return std::make_pair(std::move(dfa), std::move(dfaToNfaMap));
}

NFA NFA::copy(std::unordered_map<State *, State *> *uMap /* = nullptr */) const
{
	NFA result;
	std::unordered_map<State *, State *> mapping;

	std::for_each(states_begin(), states_end(), [&](auto &s1){
		auto *s2 = result.createState();
		mapping[&*s1] = s2;
		if (s1->isStarting())
			result.makeStarting(s2);
		if (s1->isAccepting())
			result.makeAccepting(s2);
	});

	std::for_each(states_begin(), states_end(), [&](auto &s1){
		std::for_each(s1->out_begin(), s1->out_end(), [&](auto &t){
			result.addTransition(mapping[&*s1], t.copyTo(mapping[t.dest]));
		});
	});
	if (uMap)
		*uMap = std::move(mapping);
	return result;
}

void NFA::breakIntoMultiple(State *s, const Transition &t)
{
	if (t.label.isPredicate())
		return;

	auto *curr = s;

	curr = addTransitionToFresh(curr, TransLabel(std::nullopt, t.label.getPreChecks()));
	curr = addTransitionToFresh(curr, TransLabel(t.label.getRelation()));
	addTransition(curr, Transition(TransLabel(std::nullopt, t.label.getPostChecks()), t.dest));
}

NFA &NFA::breakToParts()
{
	std::vector<std::pair<State *, Transition>> toBreak;
	std::vector<std::pair<State *, Transition>> toRemove;
	std::for_each(states_begin(), states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](auto &t){
			if (!t.label.isPredicate()) {
				toBreak.push_back({&*s, t});
				toRemove.push_back({&*s, t});
			}
		});
	});
	std::for_each(toBreak.begin(), toBreak.end(), [&](auto &p){
		breakIntoMultiple(p.first, p.second);
	});
	std::for_each(toRemove.begin(), toRemove.end(), [&](auto &p){
		removeTransition(p.first, p.second);
	});
	removeDeadStates();
	return *this;
}

// Return the state composition matrix, which is useful for minimizing the
// states of an NFA.  See Definition 3 of Kameda and Weiner: On the State
// Minimization of Nondeterministic Finite Automata
std::unordered_map<NFA::State *, std::vector<char>> NFA::get_state_composition_matrix ()
{
	flip();
	auto p = to_DFA();
	auto &dfa = p.first;
	auto &dfaToNfaMap = p.second;
	flip();

	std::unordered_map<State *, std::vector<char>> result;


	KATER_DEBUG(
		std::cout << "State composition matrix: " << std::endl;
	);
	std::for_each(states_begin(), states_end(), [&](auto &si){
		std::vector<char> row(dfaToNfaMap.size(), 0);
		auto i = 0u;
		std::for_each(dfaToNfaMap.begin(), dfaToNfaMap.end(), [&](auto &kv){
			if (kv.second.find(&*si) != kv.second.end())
				row[i] = 1;
			++i;
		});
		result.insert({&*si, row});
		KATER_DEBUG(
			std::cout << row << ": " << si->getId() << std::endl;
		);
	});
	return result;
}


// Reduce the NFA using the state composition matrix (cf. Kameda and Weiner)
void NFA::scm_reduce ()
{
	simplify_basic();
	if (getNumStates() == 0)
		return;

	auto scm = get_state_composition_matrix();
	auto dfaSize = scm.begin()->second.size();
	std::vector<State *> toRemove;
	for (auto itI = states_begin(); itI != states_end(); /* ! */) {
		if (isStarting(itI->get())) {
			++itI;
			continue;
		}
		/* Is I equal to the union of some other rows? */
		std::vector<char> newrow(dfaSize, 0);
		for (auto itJ = states_begin(); itJ != states_end(); ++itJ) {
			if (itI->get() != itJ->get() && is_subset(scm[itJ->get()], scm[itI->get()]))
				take_union(newrow, scm[itJ->get()]);
		}
		/* If not, skip, otherwise, erase */
		if (newrow != scm[itI->get()]) {
			++itI;
			continue;
		}
		KATER_DEBUG(
			std::cout << "erase node " << (*itI)->getId() << " with";
		);
		for (auto itJ = states_begin(); itJ != states_end(); ++itJ) {
			if (itI->get() != itJ->get() && is_subset(scm[itJ->get()], scm[itI->get()])) {
				KATER_DEBUG(
					std::cout << " " << (*itJ)->getId();
				);
				addInvertedTransitions(itJ->get(), (*itI)->in_begin(), (*itI)->in_end());
			}
		}
		KATER_DEBUG(
			std::cout << std::endl;
		);
		scm.erase(itI->get());
		itI = removeState(itI);
	}
}

NFA &NFA::addTransitivePredicateEdges(bool removeOld /* = true */)
{
	std::vector<std::pair<State *, Transition>> toRemove;
	std::vector<Transition> toCreateStarting;
	std::unordered_map<State *, std::vector<Transition>> toDuplicateAccepting;

	for (auto it = states_begin(); it != states_end(); ++it) {
		auto &s = *it;
		std::vector<Transition> toAdd;
		for (auto outIt = s->out_begin(); outIt != s->out_end(); ++outIt) {
			if (!outIt->label.isPredicate())
				continue;

			for (auto outIt2 = outIt->dest->out_begin(); outIt2 != outIt->dest->out_end(); ++outIt2) {
				if (!outIt2->label.isPredicate())
					continue;

				auto l = outIt->label;
				if (l.merge(outIt2->label)) {
					auto trans = Transition(l, outIt2->dest);
					toAdd.push_back(trans);
					if (isAccepting(outIt->dest)) {
						toDuplicateAccepting[outIt2->dest].push_back(
							trans.flipTo(&*s));
					}
				}
				if (isStarting(outIt->dest))
					toCreateStarting.push_back(*outIt2);
				toRemove.push_back({outIt->dest, *outIt2});
			}
		}
		addTransitions(&*s, toAdd.begin(), toAdd.end());
	}
	if (!toCreateStarting.empty()) {
		auto *n = createStarting();
		addTransitions(n, toCreateStarting.begin(), toCreateStarting.end());
	}
	std::for_each(toRemove.begin(), toRemove.end(), [&](auto &p){
		removeTransition(p.first, p.second);
	});
	std::for_each(toDuplicateAccepting.begin(), toDuplicateAccepting.end(), [&](auto &kv){
		if (std::all_of(kv.first->in_begin(), kv.first->in_end(), [&](auto &t){
					return std::find(kv.second.begin(), kv.second.end(), t) != kv.second.end(); }))
			makeAccepting(kv.first);
		else {
			auto shouldNotAcceptTrans = [&](auto &t){
				return std::find(kv.second.begin(), kv.second.end(), t) == kv.second.end();
			};
			auto *d = splitState(kv.first, shouldNotAcceptTrans);
			makeAccepting(d);
		}
	});
	return *this;
}

/* Join `[...]` edges with successor edges */
bool NFA::joinPredicateEdges(std::function<bool(const TransLabel &)> isValidTransition)
{
	bool changed = false;
	std::for_each(states_begin(), states_end(), [&](auto &s){
		std::vector<Transition> toRemove;
		std::copy_if(s->out_begin(), s->out_end(), std::back_inserter(toRemove), [&](const Transition &t){
			if (!t.label.isPredicate())
				return false;
			if (isAccepting(t.dest) && t.dest != &*s)
				return false;
			KATER_DEBUG(
				std::cout << "Compacting edge " << s->getId() << " --"
					  << t.label << "--> " << t.dest->getId() << std::endl;
			);
			if (t.dest != &*s) {
				std::for_each(t.dest->out_begin(), t.dest->out_end(), [&](const Transition &q){
					auto l = t.label;
					if (l.merge(q.label, isValidTransition))
						addTransition(&*s, Transition(l, q.dest));
				});
			}
			return true;
		});
		changed |= !toRemove.empty();
		removeTransitions(&*s, toRemove.begin(), toRemove.end());
	});
	return changed;
}

void NFA::removeRedundantSelfLoops()
{
	std::for_each(states_begin(), states_end(), [&](auto &s){
		std::vector<Transition> toRemove;
		std::copy_if(s->out_begin(), s->out_end(), std::back_inserter(toRemove), [&](const Transition &t1){
			return (t1.dest != &*s && !isAccepting(&*s) &&
			    std::all_of(s->out_begin(), s->out_end(), [&](const Transition &t2){
					    return t2.label == t1.label &&
						    (t2.dest == &*s ||
						     std::find(t1.dest->out_begin(),
							       t1.dest->out_end(), t2) != t1.dest->out_end());
				    }));
		});
		std::transform(toRemove.begin(), toRemove.end(), toRemove.begin(),
			       [&](auto &t){ return Transition(t.label, &*s); });
		removeTransitions(&*s, toRemove.begin(), toRemove.end());
	});
}

void NFA::compactEdges(std::function<bool(const TransLabel &)> isValidTransition)
{
	while (joinPredicateEdges(isValidTransition))
		;
	removeRedundantSelfLoops();
}

bool isSimilarTo(const NFA &nfa, const NFA::Transition &t1, const NFA::Transition &t2,
		 const std::unordered_map<NFA::State *, std::unordered_map<NFA::State *, bool>> &similar)
{
	return t1.label == t2.label &&
		similar.at(t1.dest).at(t2.dest);
}

bool hasSimilarTransition(const NFA &nfa, NFA::State *s, const NFA::Transition &t1,
			  const std::unordered_map<NFA::State *, std::unordered_map<NFA::State *, bool>> &similar)
{
	return std::any_of(s->out_begin(), s->out_end(), [&](auto &t2){
			return isSimilarTo(nfa, t1, t2, similar);
		});
}

bool isSimilarTo(const NFA &nfa, NFA::State *s1, NFA::State *s2,
		 const std::unordered_map<NFA::State *, std::unordered_map<NFA::State *, bool>> &similar)
{
	return std::all_of(s1->out_begin(), s1->out_end(), [&](auto &t1){
			return nfa.hasTransition(s2, t1) || hasSimilarTransition(nfa, s2, t1, similar);
		});
}

std::unordered_map<NFA::State *, std::unordered_map<NFA::State *, bool>>
NFA::findSimilarStates() const
{
	std::unordered_map<State *, std::unordered_map<State *, bool>> similar;

	std::unordered_map<State *, bool> initV;
	std::generate_n(std::inserter(initV, initV.begin()), getNumStates(),
			[sIt = states_begin()] () mutable { return std::make_pair(&**sIt++, true); });
	std::generate_n(std::inserter(similar, similar.begin()), getNumStates(),
			[sIt = states_begin(), v = initV] () mutable { return std::make_pair(&**sIt++, v); });

	/* Remove accepting/non-accepting pairs */
	std::for_each(accept_begin(), accept_end(), [&](auto &a){
			std::for_each(states_begin(), states_end(), [&](auto &s){
			if (!isAccepting(&*s))
				similar[a][&*s] = false;
		});
	});

	bool changed = true;
	while (changed) {
		changed = false;
		std::for_each(states_begin(), states_end(), [&](auto &s1){
			std::for_each(states_begin(), states_end(), [&](auto &s2){
				if (similar[&*s1][&*s2] && !isSimilarTo(*this, &*s1, &*s2, similar)) {
					similar[&*s1][&*s2] = false;
					changed = true;
				}
			});
		});
	}
	return similar;
}

void NFA::removeSimilarTransitions()
{
	auto similar = findSimilarStates();

	/* Bisimilar states */
	std::vector<State *> toRemove;
	std::for_each(states_begin(), states_end(), [&](auto &s1){
		std::for_each(states_begin(), states_end(), [&](auto &s2){
			if (&*s1 != &*s2 && similar[&*s1][&*s2] && similar[&*s2][&*s1] &&
			    (isAccepting(&*s1) == isAccepting(&*s2))) {
				addInvertedTransitions(&*s1, s2->in_begin(), s2->in_end());
				toRemove.push_back(&*s2);
			}
		});
	});
	removeStates(toRemove.begin(), toRemove.end());

	/* Transitions to similar states */
	std::for_each(states_begin(), states_end(), [&](auto &s){
		removeTransitionsIf(&*s, [&](auto &t1){
			return std::any_of(s->out_begin(), s->out_end(), [&](auto &t2){
					return t1 != t2 && isSimilarTo(*this, t1, t2, similar);
				});
		});
	});
	removeDeadStates();
}

NFA &NFA::simplify(std::function<bool(const TransLabel &)> isValidTransition)
{
	simplify_basic();
	KATER_DEBUG(std::cout << "After first simplification: " << *this;);

	applyBidirectionally([&](){ scm_reduce(); });
	KATER_DEBUG(std::cout << "After 1st SCM reduction: " << *this;);

	applyBidirectionally([&](){ compactEdges(isValidTransition); });
	KATER_DEBUG(std::cout << "After edge compaction: " << *this;);

	applyBidirectionally([&](){ scm_reduce(); });
	KATER_DEBUG(std::cout << "After 2nd SCM reduction: " << *this;);

	// applyBidirectionally([&](){ removeSimilarTransitions(); });
	// KATER_DEBUG(std::cout << "After similar-transition removal: " << *this;);

	applyBidirectionally([&](){ removeDeadStates(); });
	KATER_DEBUG(std::cout << "After dead-state removal: " << *this;);

	KATER_DEBUG(std::cout << "After last simplification: " << *this;);
	simplify_basic();
	return *this;
}

NFA &NFA::reduce(ReductionType t)
{
	simplify();
	std::for_each(accept_begin(), accept_end(), [&](auto &s){
		removeTransitionsIf(&*s, [&](const Transition &t){
			return std::any_of(start_begin(), start_end(), [&](auto &q) {
				return q->hasOutgoingTo(t.dest); });
		});
	});
	simplify();
	return *this;
}

std::unordered_set<NFA::State *>
NFA::calculateReachableFrom(const std::vector<State *> &ss) const
{
	std::unordered_set<State *> visited;
	std::vector<State *> workList;

	for (auto *s : ss) {
		visited.insert(s);
		workList.push_back(s);
	}
	while (!workList.empty()) {
		auto *s = workList.back();
		workList.pop_back();
		for (auto it = s->out_begin(); it != s->out_end(); it++) {
			if (visited.count(it->dest))
				continue;
			visited.insert(it->dest);
			workList.push_back(it->dest);
		}
	}
	return visited;
}

std::unordered_set<NFA::State *>
NFA::calculateReachingTo(const std::vector<State *> &ss)
{
	flip();
	auto visited = calculateReachableFrom(ss);
	flip();
	return visited;
}

std::unordered_set<NFA::State *>
NFA::calculateUsefulStates()
{
	return calculateReachingTo(std::vector<State *>(getAccepting()));
}

void NFA::removeDeadStatesDFS()
{
	auto useful = calculateUsefulStates();

	std::vector<State *> toRemove;
	std::for_each(states_begin(), states_end(), [&](auto &s){
		if (!useful.count(&*s))
			toRemove.push_back(&*s);
	});
	removeStates(toRemove.begin(), toRemove.end());

	if (getNumStarting() == 0)
		createStarting();
	return;
}

NFA &NFA::removeDeadStates()
{
	applyBidirectionally([this](){ removeDeadStatesDFS(); });
	return *this;
}

template<typename T>
std::ostream & operator<< (std::ostream& ostr, const std::set<T> &s)
{
	bool not_first = false;
	for (auto i : s) {
		if (not_first) ostr << ", ";
		else not_first = true;
		ostr << i;
	}
	return ostr;
}

template<>
std::ostream & operator<< (std::ostream& ostr, const std::set<NFA::State *> &s)
{
	bool not_first = false;
	for (auto i : s) {
		if (not_first) ostr << ", ";
		else not_first = true;
		ostr << i->getId();
	}
	return ostr;
}

std::ostream & operator<< (std::ostream& ostr, const NFA& nfa)
{
	ostr << "[NFA with " << nfa.getNumStates() << " states]" << std::endl;
	ostr << "starting:";
	std::for_each(nfa.start_begin(), nfa.start_end(), [&](auto &s){
		ostr << " " << s->getId();
	});
	ostr << " accepting:";
	std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &s){
		ostr << " " << s->getId();
	});
	ostr << std::endl;
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](const NFA::Transition &t){
			ostr << "\t" << s->getId() << " --" << t.label << "--> " << t.dest->getId() << std::endl;
		});
	});
	return ostr;
}
