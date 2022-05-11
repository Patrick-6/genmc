#include "Config.hpp"
#include "Error.hpp"
#include "NFA.hpp"
#include <fstream>
#include <iostream>

#define DEBUG_TYPE "nfa"

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

bool NFA::acceptsNoString(std::string &cex) const
{
	std::unordered_set<State *> visited;
	std::vector<std::pair<State *, std::string>> workList;

	for (auto it = states_begin(); it != states_end(); it++) {
		if (!isStarting(it->get()))
			continue;
		visited.insert(it->get());
		workList.push_back({it->get(), ""});
	}
	while (!workList.empty()) {
		auto &p = workList.back();
		auto s = p.first;
		workList.pop_back();
		if (isAccepting(s)) {
			cex = p.second;
			return false;
		}
		for (auto it = s->out_begin(); it != s->out_end(); it++) {
			if (visited.count(it->dest))
				continue;
			visited.insert(it->dest);
			workList.push_back({it->dest, p.second + " " + it->label.toString()});
		}
	}
	return true;
}

bool NFA::isSubLanguageOfDFA(const NFA &other, std::string &cex) const
{
	KATER_DEBUG(
		std::cout << "Checking inclusion between automata:" << std::endl;
		std::cout << *this << "and " << other << std::endl;
	);

	if (other.getNumStarting() == 0)
		return acceptsNoString(cex);

	using SPair = std::pair<State *, State *>;
	struct SPairHasher {
		inline void hash_combine(std::size_t& seed, std::size_t v) const {
			seed ^= v + 0x9e3779b9 + (seed<<6) + (seed>>2);
		}
		std::size_t operator()(SPair p) const {
			std::size_t hash = 0;
			hash_combine(hash, p.first->getId());
			hash_combine(hash, p.second->getId());
			return hash;
		}
	};

	std::unordered_set<SPair, SPairHasher> visited;
	std::vector<std::pair<SPair, std::string>> workList;

	for (auto it = states_begin(); it != states_end(); it++) {
		if (!isStarting(it->get()))
			continue;
		for (auto oit = other.states_begin(); oit != other.states_end(); oit++) {
			if (!isStarting(oit->get()))
				continue;
			visited.insert({it->get(), oit->get()});
			workList.push_back({{it->get(), oit->get()}, ""});
		}
	}
	while (!workList.empty()) {
		auto [p,str] = workList.back();
		workList.pop_back();
		if (isAccepting(p.first) && !isAccepting(p.second)) {
			cex = str;
			return false;
		}

		for (auto it = p.first->out_begin(); it != p.first->out_end(); it++) {
			std::string new_str = str + " ";
			KATER_DEBUG(
				new_str += std::to_string(p.first->getId()) + "/" +
					   std::to_string(p.second->getId()) + " ";
			);
			new_str += it->label.toString();
			bool found_edge = false;
			for (auto oit = p.second->out_begin(); oit != p.second->out_end(); oit++) {
				if (it->label != oit->label) continue;
				found_edge = true;
				if (visited.count({it->dest, oit->dest})) continue;
				visited.insert({it->dest, oit->dest});
				workList.push_back({{it->dest, oit->dest}, new_str});
			}
			if (!found_edge) {
				cex = new_str;
				return false;
			}
		}
	}
	return true;
}

NFA::NFA(const TransLabel &c) : NFA()
{
	auto *init = createStarting();
	auto *fnal = createAccepting();
	addTransition(init, Transition(c, fnal));
	return;
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
			addEpsilonTransition(a, s);
		});
	});

	/* Clear accepting states if necessary */
	if (!other.acceptsEmptyString())
		clearAccepting();

	/* Clear starting states of `other` */
	other.clearStarting();

	/* Move the states of the `other` NFA into our NFA and append accepting states */
	std::move(other.states_begin(), other.states_end(), std::back_inserter(nfa));
	getAccepting().insert(getAccepting().end(), other.getAccepting().begin(), other.getAccepting().end());
	return *this;
}

NFA &NFA::plus()
{
	/* Add transitions `this->accepting --> other.starting.outgoing` */
	std::for_each(accept_begin(), accept_end(), [&](auto &a){
		std::for_each(start_begin(), start_end(), [&](auto &s){
			addEpsilonTransition(a, s);
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
	return plus().or_empty();
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
			return (t1.dest != &*s &&
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

NFA &NFA::simplify(std::function<bool(const TransLabel &)> isValidTransition)
{
	simplify_basic();
	KATER_DEBUG(std::cout << "After basic simplification: " << *this;);
	scm_reduce();
	KATER_DEBUG(std::cout << "After 1st SCM reduction: " << *this;);
	flip();
	scm_reduce();
	flip();
	KATER_DEBUG(std::cout << "After 2nd SCM reduction: " << *this;);
	compactEdges(isValidTransition);
	flip();
	compactEdges(isValidTransition);
	flip();
	KATER_DEBUG(std::cout << "After edge compaction: " << *this;);
	scm_reduce();
	KATER_DEBUG(std::cout << "After 3rd SCM reduction: " << *this;);
	flip();
	scm_reduce();
	KATER_DEBUG(std::cout << "Flipped: " << *this;);
	flip();
	KATER_DEBUG(std::cout << "After 4th SCM reduction: " << *this;);
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
