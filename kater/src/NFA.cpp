#include "Config.hpp"
#include "NFA.hpp"
#include <fstream>
#include <iostream>

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
			if (((*it)->hasAllOutLoops() && !(*it)->isAccepting()) ||
			    ((*it)->hasAllInLoops() && !(*it)->isStarting())) {
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
				if ((*it)->isAccepting() == (*oit)->isAccepting() &&
				    (*it)->outgoingSameAs(oit->get())) {
					if ((*oit)->isStarting())
						(*it)->setStarting(true);
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
				if ((*it)->isStarting() == (*oit)->isStarting() &&
				    (*it)->incomingSameAs(oit->get())) {
					if ((*oit)->isAccepting())
						(*it)->setAccepting(true);
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
	return std::any_of(states_begin(), states_end(), [](auto &s){
		return s->isStarting() && s->isAccepting();
	});
}

bool NFA::acceptsNoString(std::string &cex) const
{
	std::unordered_set<const State *> visited;
	std::vector<std::pair<const State *, std::string>> workList;

	for (auto it = states_begin(); it != states_end(); it++) {
		if (!it->get()->isStarting()) continue;
		visited.insert(it->get());
		workList.push_back({it->get(), ""});
	}
	while (!workList.empty()) {
		auto &p = workList.back();
		auto s = p.first;
		workList.pop_back();
		if (s->isAccepting()) {
			cex = p.second;
			return false;
		}
		for (auto it = s->out_begin(); it != s->out_end(); it++) {
			if (visited.count(it->dest)) continue;
			visited.insert(it->dest);
			workList.push_back({it->dest, p.second + " " + it->label->toString()});
		}
	}
	return true;
}

bool NFA::isSubLanguageOfDFA(const NFA &other, std::string &cex) const
{
	if (config.verbose > 0) {
		std::cout << "Checking inclusion between automata:" << std::endl;
		std::cout << *this << "and " << other << std::endl;
	}

	// If the second automaton has no starting states...
	if (std::none_of (other.states_begin(), other.states_end(),
			 [](auto &s) { return s->isStarting(); }))
		return acceptsNoString(cex);

	using SPair = std::pair<const State *, const State *>;
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
		if (!it->get()->isStarting())
			continue;
		for (auto oit = other.states_begin(); oit != other.states_end(); oit++) {
			if (!oit->get()->isStarting())
				continue;
			visited.insert({it->get(), oit->get()});
			workList.push_back({{it->get(), oit->get()}, ""});
		}
	}
	while (!workList.empty()) {
		auto [p,str] = workList.back();
		workList.pop_back();
		if (p.first->isAccepting() && !p.second->isAccepting()) {
			cex = str;
			return false;
		}

		for (auto it = p.first->out_begin(); it != p.first->out_end(); it++) {
			std::string new_str = str + " ";
			if (config.verbose > 0)
				new_str += std::to_string(p.first->getId()) + "/" +
					   std::to_string(p.second->getId()) + " ";
			new_str += it->label->toString();
			bool found_edge = false;
			for (auto oit = p.second->out_begin(); oit != p.second->out_end(); oit++) {
				if (*it->label != *oit->label) continue;
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
	addTransition(init, Transition(c.clone(), fnal));
	return;
}

NFA &NFA::alt(NFA &&other)
{
	/* Move the states of the `other` NFA into our NFA */
	std::move(other.states_begin(), other.states_end(), std::back_inserter(nfa));
	return *this;
}

NFA &NFA::seq(NFA &&other)
{
	/* Add transitions `this->accepting --> other.starting.outgoing` */
	std::for_each(states_begin(), states_end(), [&](auto &a){
		if (a->isAccepting())
			std::for_each(other.states_begin(), other.states_end(), [&](auto &s){
				if (s->isStarting())
					addTransitions(a.get(), s->out_begin(), s->out_end());
			});
	});

	/* Clear accepting states if necessary */
	if (!other.acceptsEmptyString())
		std::for_each(states_begin(), states_end(), [&](auto &s){ s->setAccepting(false); });

	/* Clear starting states of `other` */
	std::for_each(other.states_begin(), other.states_end(), [&](auto &s){ s->setStarting(false); });

	/* Move the states of the `other` NFA into our NFA */
	std::move(other.states_begin(), other.states_end(), std::back_inserter(nfa));
	return *this;
}

NFA &NFA::plus()
{
	/* Add transitions `this->accepting --> other.starting.outgoing` */
	std::for_each(states_begin(), states_end(), [&](auto &a){
		if (a->isAccepting())
			std::for_each(states_begin(), states_end(), [&](auto &s){
				if (s->isStarting())
					addTransitions(a.get(), s->out_begin(), s->out_end());
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
	auto it = std::find_if(states_begin(), states_end(), [](auto &s){
			return s->isStarting() && !s->hasIncoming();});

	auto *s = (it != states_end()) ? it->get() : createStarting();
	s->setAccepting(true);
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
	std::set<State *> ss;
	std::for_each(states_begin(), states_end(), [&](const auto &a){
			if (a->isStarting()) ss.insert(a.get()); });
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
						if (*t2.label == *t.label)
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
				dfa.addTransition(nfaToDfaMap[sc], Transition(t.label->clone(), ds));
			});
		});
	}

	std::for_each(dfaToNfaMap.begin(), dfaToNfaMap.end(), [&](auto &kv){
		if (std::any_of(kv.second.begin(), kv.second.end(), [&](State *s){
			return s->isAccepting();
		}))
			kv.first->setAccepting(true);
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

	if (config.verbose > 1)
		std::cout << "State composition matrix: " << std::endl;
	std::for_each(states_begin(), states_end(), [&](auto &si){
		std::vector<char> row(dfaToNfaMap.size(), 0);
		auto i = 0u;
		std::for_each(dfaToNfaMap.begin(), dfaToNfaMap.end(), [&](auto &kv){
			if (kv.second.find(&*si) != kv.second.end())
				row[i] = 1;
			++i;
		});
		result.insert({&*si, row});
		if (config.verbose > 1)
			std::cout << row << ": " << si->getId() << std::endl;
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
		if ((*itI)->isStarting()) {
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
		if (config.verbose > 1)
			std::cout << "erase node " << (*itI)->getId() << " with";
		for (auto itJ = states_begin(); itJ != states_end(); ++itJ) {
			if (itI->get() != itJ->get() && is_subset(scm[itJ->get()], scm[itI->get()])) {
				if (config.verbose > 1)
					std::cout << " " << (*itJ)->getId();
				addInvertedTransitions(itJ->get(), (*itI)->in_begin(), (*itI)->in_end());
			}
		}
		if (config.verbose > 1) std::cout << std::endl;
		scm.erase(itI->get());
		itI = removeState(itI);
	}
}

void NFA::compact_edges()
{
	/* Join `[...]` edges with successor edges */
//	std::for_each(states_begin(), states_end(), [&](auto &s){
//		std::vector<Transition> toRemove;
//		std::copy_if(s->out_begin(), s->out_end(), std::back_inserter(toRemove), [&](const Transition &t){
//			if (!t.label.is_empty_trans())
//				return false;
//			if (isAccepting(t.dest) && t.dest != &*s)
//				return false;
//			if (config.verbose > 1) {
//				std::cout << "Compacting edge " << s->getId() << " --"
//					  << t.label << "--> " << t.dest->getId() << std::endl;
//			}
//			if (t.dest != &*s) {
//				std::for_each(t.dest->out_begin(), t.dest->out_end(), [&](const Transition &q){
//					auto l = t.label.seq(q.label);
//					if (l.is_valid())
//						addTransition(&*s, Transition(l, q.dest));
//				});
//			}
//			return true;
//		});
//		removeTransitions(&*s, toRemove.begin(), toRemove.end());
//	});

	/* Remove redundant self loops */
	std::for_each(states_begin(), states_end(), [&](auto &s){
		std::vector<Transition> toRemove;
		std::copy_if(s->out_begin(), s->out_end(), std::back_inserter(toRemove), [&](const Transition &t1){
			return (t1.dest != &*s &&
			    std::all_of(s->out_begin(), s->out_end(), [&](const Transition &t2){
					    return *t2.label == *t1.label &&
						    (t2.dest == &*s || std::find(t1.dest->out_begin(),
										 t1.dest->out_end(), t2) != t1.dest->out_end());
				    }));
		});
		std::for_each(toRemove.begin(), toRemove.end(), [&](const Transition &t){
			removeTransition(&*s, Transition(t.label->clone(), &*s));
		});
	});
}


NFA &NFA::simplify ()
{
	simplify_basic();
	if (config.verbose > 1) std::cout << "After basic simplification: " << *this;
	scm_reduce();
	if (config.verbose > 1) std::cout << "After 1st SCM reduction: " << *this;
	flip();
	scm_reduce();
	flip();
	if (config.verbose > 1) std::cout << "After 2nd SCM reduction: " << *this;
	compact_edges();
	flip();
	compact_edges();
	flip();
	if (config.verbose > 1) std::cout << "After edge compaction: " << *this;
	scm_reduce();
	if (config.verbose > 1) std::cout << "After 3rd SCM reduction: " << *this;
	flip();
	scm_reduce();
	if (config.verbose > 2) std::cout << "Flipped: " << *this;
	flip();
	if (config.verbose > 1) std::cout << "After 4th SCM reduction: " << *this;
	simplify_basic();
	return *this;
}

NFA &NFA::simplifyReduce()
{
	star().simplify();
	if (std::count_if(states_begin(), states_end(), [](auto &s){
			return s->isStarting() || s->isAccepting();}) != 1)
		return *this;

	auto *sold = std::find_if(states_begin(), states_end(), [](auto &s){
			return s->isStarting();})->get();
	sold->setStarting(false);
	auto *snew = createStarting();
	addTransitions(snew, sold->out_begin(), sold->out_end());

	removeAllTransitions(sold);
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
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		if (s->isStarting()) ostr << " " << s->getId(); });
	ostr << " accepting:";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		if (s->isAccepting()) ostr << " " << s->getId(); });
	ostr << std::endl;
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](const NFA::Transition &t){
			ostr << "\t" << s->getId() << " --" << *t.label << "--> " << t.dest->getId() << std::endl;
		});
	});
	return ostr;
}


// ------------------------------------------------------------
// Generate C++ code for integration with GenMC
// ------------------------------------------------------------
#define PRINT_LINE(line) fout << line << "\n"

// Macros for calculators
#define VSET             "VSet<Event>"
#define CALC             "calculate" << whichCalc << "(const ExecutionGraph &g, const Event &e)"
#define VISIT_PROC(i)    "visit" << whichCalc << "_" << i
#define VISIT_CALL(i,e)  VISIT_PROC(i) << "(calcRes, " << e << ");"
#define VISIT_PARAMS	 "(" << VSET << " &calcRes, const ExecutionGraph &g, const Event &e)"
#define VISITED_ARR	 "visitedCalc" << whichCalc
#define VISITED_IDX(i,e) VISITED_ARR << "[g.getEventLabel(" << e \
			 << ")->getStamp()][" << i << "]"

void NFA::print_calculator_header_public (std::ostream &fout, int whichCalc) const
{
	PRINT_LINE("\t" << VSET << " " << CALC << ";");
}

template<typename ITER>
std::unordered_map<NFA::State *, unsigned> assignStateIDs(ITER &&begin, ITER &&end)
{
	std::unordered_map<NFA::State *, unsigned> result;

	auto i = 0u;
	std::for_each(begin, end, [&](auto &s){ result[&*s] = i++; });
	return result;
}

void NFA::print_calculator_header_private (std::ostream &fout, int whichCalc) const
{
	auto ids = assignStateIDs(states_begin(), states_end());
	std::for_each(states_begin(), states_end(), [&](auto &s){
		PRINT_LINE("\tvoid " << VISIT_PROC(ids[&*s]) << VISIT_PARAMS);
	});

	PRINT_LINE("\tstd::vector<std::bitset<" << getNumStates() <<  "> > " << VISITED_ARR << ";");
}

void NFA::printCalculatorImplHelper(std::ostream &fout, const std::string &className,
				    int whichCalc, bool reduce) const
{
	auto ids = assignStateIDs(states_begin(), states_end());

	std::for_each(states_begin(), states_end(), [&](auto &s){
		PRINT_LINE("void " << className << "::" << VISIT_PROC(ids[&*s]) << VISIT_PARAMS);
		PRINT_LINE("{");
		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("");

		PRINT_LINE("\t" << VISITED_IDX(ids[&*s],"e") << " = true;");
		if (s->isStarting()) {
			PRINT_LINE("\tcalcRes.insert(e);");
			if (reduce) {
				PRINT_LINE("\tfor (const auto &p : calc" << whichCalc << "_preds(g, e)) {");
				PRINT_LINE("\t\tcalcRes.erase(p);");
				std::for_each(states_begin(), states_end(), [&](auto &a){
					if (a->isAccepting())
						PRINT_LINE("\t\t" << VISITED_IDX(ids[&*a], "p") << " = true;");
				});
				PRINT_LINE("\t}");
			}
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(fout, "e", "p");
			PRINT_LINE("\t\tif (" << VISITED_IDX(ids[t.dest], "p") << ") continue;");
			PRINT_LINE("\t\t" << VISIT_CALL(ids[t.dest], "p"));
			PRINT_LINE("\t}");
		});
		PRINT_LINE("}");
		PRINT_LINE("");
	});

	PRINT_LINE(VSET << " " << className << "::" << CALC);
	PRINT_LINE("{");
	PRINT_LINE("\t" << VSET << " calcRes;");
	PRINT_LINE("\t" << VISITED_ARR << ".clear();");
	PRINT_LINE("\t" << VISITED_ARR << ".resize(g.getMaxStamp() + 1);");
	std::for_each(states_begin(), states_end(), [&](auto &a){
		if (a->isAccepting())
			PRINT_LINE("\t" << VISIT_CALL(ids[&*a], "e"));
	});
	PRINT_LINE("\treturn calcRes;");
	PRINT_LINE("}");
}

#undef VSET
#undef CALC
#undef VISIT_PROC
#undef VISIT_CALL
#undef VISIT_PARAMS
#undef VISITED_ARR
#undef VISITED_IDX

// Macros for acyclicity checks
#define VISIT_PROC(i)      "visitAcyclic" << i
#define VISIT_CALL(i,e)    VISIT_PROC(i) << "(" << e << ")"
#define VISIT_PARAMS	   "(const Event &e)"
#define IS_CONS            "isConsistent" << VISIT_PARAMS
#define VISITED_ARR	   "visitedAcyclic"
#define VISITED_IDX(i,e)   VISITED_ARR << i << "[g.getEventLabel(" << e << ")->getStamp()]"
#define VISITED_ACCEPTING  "visitedAccepting"

void NFA::print_acyclic_header_public (std::ostream &fout) const
{
	PRINT_LINE("\tbool " << IS_CONS << ";");
}

void NFA::print_acyclic_header_private (std::ostream &fout) const
{
	auto ids = assignStateIDs(states_begin(), states_end());

	/* visit procedures */
	PRINT_LINE("");
	std::for_each(states_begin(), states_end(), [&](auto &s){
		PRINT_LINE("\tbool " << VISIT_PROC(ids[&*s]) << VISIT_PARAMS << ";");
	});

	/* status arrays */
	PRINT_LINE("");
	std::for_each(states_begin(), states_end(), [&](auto &s){
		PRINT_LINE("\tstd::vector<NodeStatus> " << VISITED_ARR << ids[&*s] << ";");
	});

	/* accepting counter */
	PRINT_LINE("");
	PRINT_LINE("\tint " <<  VISITED_ACCEPTING << ";");
}

void NFA::print_acyclic_impl (std::ostream &fout, const std::string &className) const
{
	auto ids = assignStateIDs(states_begin(), states_end());

	std::for_each(states_begin(), states_end(), [&](auto &s){
		PRINT_LINE("bool " << className << "::" << VISIT_PROC(ids[&*s]) << VISIT_PARAMS);
		PRINT_LINE("{");

		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("\tauto *lab = g.getEventLabel(" << "e" << ");");

		PRINT_LINE("");
		if (s->isStarting())
			PRINT_LINE("\t++" << VISITED_ACCEPTING << ";");
		PRINT_LINE("\t" << VISITED_ARR << ids[&*s] << "[lab->getStamp()]" << " = NodeStatus::entered;");
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(fout, "e", "p");
			PRINT_LINE("\t\tauto status = " << VISITED_IDX(ids[t.dest], "p") << ";");
			PRINT_LINE("\t\tif (status == NodeStatus::unseen && !" << VISIT_CALL(ids[t.dest], "p") << ")");
			PRINT_LINE("\t\t\treturn false;");
			PRINT_LINE("\t\telse if (status == NodeStatus::entered)");
			PRINT_LINE("\t\t\treturn false;");
			PRINT_LINE("\t}");
		});
		if (s->isStarting())
			PRINT_LINE("\t--" << VISITED_ACCEPTING << ";");
		PRINT_LINE("\treturn true;");
		PRINT_LINE("}");
		PRINT_LINE("");
	});

	PRINT_LINE("bool " << className << "::" << IS_CONS);
	PRINT_LINE("{");
	PRINT_LINE("\t" << VISITED_ACCEPTING << " = 0;");
	PRINT_LINE("\t" << VISITED_ARR << ".clear();");
	PRINT_LINE("\t" << VISITED_ARR << ".resize(g.getMaxStamp() + 1);");
	PRINT_LINE("\treturn true");
	std::for_each(states_begin(), states_end(), [&](auto &s){
		PRINT_LINE ("\t\t&& " << VISIT_CALL(ids[&*s], "e")
			    << (&*s == (--states_end())->get() ? ";" : ""));
	});
	PRINT_LINE("}");
	PRINT_LINE("");
}
