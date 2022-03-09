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

		for (auto it = states_begin(), ie = states_end(); it != ie; /* */ ) {
			if (((*it)->hasAllOutLoops() && !isAccepting(it->get())) ||
			    ((*it)->hasAllInLoops() && !isStarting(it->get()))) {
				it = removeState(it);
				changed = true;
			} else {
				++it;
			}
		}

		for (auto it = states_begin(), ie = states_end(); it != ie; ++it) {
			auto oit(it);
			++oit;
			for (auto oe = states_end(); oit != oe; /* */) {
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

		for (auto it = states_begin(), ie = states_end(); it != ie; ++it) {
			auto oit(it);
			++oit;
			for (auto oe = states_end(); oit != oe; /* */) {
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

NFA::NFA(const TransLabel &c) : NFA()
{
	auto *init = createStarting();
	auto *fnal = createAccepting();
	addTransition(init, Transition(c, fnal));
	return;
}

NFA &NFA::alt(NFA &&other)
{
	getStates().merge(std::move(other.getStates()));
	getStarting().merge(std::move(other.getStarting()));
	getAccepting().merge(std::move(other.getAccepting()));
	return *this;
}

NFA &NFA::seq(NFA &&other)
{
	getStates().merge(std::move(other.getStates()));

	/* Add transitions `this->accepting --> other.starting`	 */
	std::for_each(accept_begin(), accept_end(), [&](State *a){
		std::for_each(other.start_begin(), other.start_end(), [&](State *s){
			addTransitions(a, s->out_begin(), s->out_end());
		});
	});

	/* Calculate accepting states */
	// XXX: ?
	if (std::none_of(other.start_begin(), other.start_end(),
			 [&](State *s){	return other.isAccepting(s); }))
		clearAccepting();
	getAccepting().merge(std::move(other.getAccepting()));
	return *this;
}

NFA &NFA::plus()
{
	std::for_each(accept_begin(), accept_end(), [&](State *a){
		std::for_each(start_begin(), start_end(), [&](State *s){
			addTransitions(a, s->out_begin(), s->out_end());
		});
	});
	return *this;
}

NFA &NFA::or_empty()
{
	// Does the NFA already accept the empty string?
	if (std::any_of(start_begin(), start_end(), [&](State *s){ return isAccepting(s); }))
		return *this;

	// Otherwise, find starting node with no incoming edges
	// XXX: Why not search starting?
	auto it = std::find_if(states_begin(), states_end(), [&](auto &s){
		return !s->hasIncoming() && isStarting(&*s);
	});

	auto *s = (it != states_end()) ? it->get() : createStarting();
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
	auto [dfa, dfaToNfaMap] = to_DFA();
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
	for (auto itI = states_begin(), itIe = states_end(); itI != itIe; /* ! */) {
		if (isStarting(itI->get())) {
			++itI;
			continue;
		}
		/* Is I equal to the union of some other rows? */
		std::vector<char> newrow(dfaSize, 0);
		for (auto itJ = states_begin(), itJe = states_end(); itJ != itJe; ++itJ) {
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
		for (auto itJ = states_begin(), itJe = states_end(); itJ != itJe; ++itJ) {
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
	std::for_each(states_begin(), states_end(), [&](auto &s){
		std::vector<Transition> toRemove;
		std::copy_if(s->out_begin(), s->out_end(), std::back_inserter(toRemove), [&](const Transition &t){
			if (!t.label.is_empty_trans())
				return false;
			if (isAccepting(t.dest) && t.dest != &*s)
				return false;
			if (config.verbose > 1) {
				std::cout << "Compacting edge " << s->getId() << " --"
					  << t.label << "--> " << t.dest->getId() << std::endl;
			}
			if (t.dest != &*s) {
				std::for_each(t.dest->out_begin(), t.dest->out_end(), [&](const Transition &q){
					auto l = t.label.seq(q.label);
					if (l.is_valid())
						addTransition(&*s, Transition(l, q.dest));
				});
			}
			return true;
		});
		removeTransitions(&*s, toRemove.begin(), toRemove.end());
	});

	/* Remove redundant self loops */
	std::for_each(states_begin(), states_end(), [&](auto &s){
		std::vector<Transition> toRemove;
		std::copy_if(s->out_begin(), s->out_end(), std::back_inserter(toRemove), [&](const Transition &t1){
			return (t1.dest != &*s &&
			    std::all_of(s->out_begin(), s->out_end(), [&](const Transition &t2){
					    return t2.label == t1.label &&
						    (t2.dest == &*s || std::find(t1.dest->out_begin(),
										 t1.dest->out_end(), t2) != t1.dest->out_end());
				    }));
		});
		std::for_each(toRemove.begin(), toRemove.end(), [&](const Transition &t){
			removeTransition(&*s, Transition(t.label, &*s));
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
	if (getNumAccepting() != 1 || getStarting() != getAccepting())
		return *this;

	auto *sold = *start_begin();
	clearStarting();
	auto *snew = createStarting();
	addTransitions(snew, sold->out_begin(), sold->out_end());

	removeAllTransitions(sold);
	simplify();
	return *this;
}

template<typename T>
static std::ostream & operator<< (std::ostream& ostr, const std::set<T> &s)
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
	ostr << "starting: {" << nfa.getStarting() << "} accepting: {" << nfa.getAccepting() << "}" << std::endl;
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		std::for_each(s->out_begin(), s->out_end(), [&](const NFA::Transition &t){
			ostr << "\t" << s->getId() << " --" << t.label << "--> " << t.dest->getId() << std::endl;
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

void NFA::print_calculator_header_public (std::ostream &fout, int whichCalc)
{
	PRINT_LINE("\t" << VSET << " " << CALC << ";");
}

void NFA::print_calculator_header_private (std::ostream &fout, int whichCalc)
{
	// for (auto i = 0u ; i < trans.size(); i++)
	// 	PRINT_LINE("\tvoid " << VISIT_PROC(i) << VISIT_PARAMS);

	// PRINT_LINE("\tstd::vector<std::bitset<" << trans.size() <<  "> > " << VISITED_ARR << ";");
}

void NFA::printCalculatorImplHelper(std::ostream &fout, const std::string &className,
				    int whichCalc, bool reduce)
{
	// for (auto i = 0u ; i < trans.size(); i++) {
	// 	PRINT_LINE("void " << className << "::" << VISIT_PROC(i) << VISIT_PARAMS);
	// 	PRINT_LINE("{");
	// 	PRINT_LINE("\tauto &g = getGraph();");
	// 	PRINT_LINE("");

	// 	PRINT_LINE("\t" << VISITED_IDX(i,"e") << " = true;");
	// 	if (is_starting (i)) {
	// 		PRINT_LINE("\tcalcRes.insert(e);");
	// 		if (reduce) {
	// 			PRINT_LINE("\tfor (const auto &p : calc" << whichCalc << "_preds(g, e)) {");
	// 			PRINT_LINE("\t\tcalcRes.erase(p);");
	// 			for (int j : accepting)
	// 				PRINT_LINE("\t\t" << VISITED_IDX(j, "p") << " = true;");
	// 			PRINT_LINE("\t}");
	// 		}
	// 	}
	// 	for (const auto &n : trans_inv[i]) {
	// 		n.first.output_as_preds(fout, "e", "p");
	// 		PRINT_LINE("\t\tif (" << VISITED_IDX(n.second, "p") << ") continue;");
	// 		PRINT_LINE("\t\t" << VISIT_CALL(n.second, "p"));
	// 		PRINT_LINE("\t}");
	// 	}
	// 	PRINT_LINE("}");
	// 	PRINT_LINE("");
	// }

	// PRINT_LINE(VSET << " " << className << "::" << CALC);
	// PRINT_LINE("{");
	// PRINT_LINE("\t" << VSET << " calcRes;");
	// PRINT_LINE("\t" << VISITED_ARR << ".clear();");
	// PRINT_LINE("\t" << VISITED_ARR << ".resize(g.getMaxStamp() + 1);");
	// for (auto &i : accepting)
	// 	PRINT_LINE("\t" << VISIT_CALL(i, "e"));
	// PRINT_LINE("\treturn calcRes;");
	// PRINT_LINE("}");
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
#define VISIT_PARAMS	   "(const ExecutionGraph &g, const Event &e)"
#define IS_CONS            "isConsistent" << VISIT_PARAMS
#define VISITED_ARR	   "visitedAcyclic"
#define VISITED_IDX(i,e)   VISITED_ARR << "[g.getEventLabel(" << e << ")->getStamp()][" << i << "]"
#define VISITED_ACCEPTING  "visitedAccepting"

void NFA::print_acyclic_header_public (std::ostream &fout)
{
	PRINT_LINE("\tbool " << IS_CONS << ";");
}

void NFA::print_acyclic_header_private (std::ostream &fout)
{
	// for (auto i = 0u ; i < trans.size(); i++)
	// 	PRINT_LINE("\tbool " << VISIT_PROC(i) << VISIT_PARAMS << ";");

	// PRINT_LINE("\tstd::vector<std::bitset< " << trans.size() <<  "> > " << VISITED_ARR << ";");
	// PRINT_LINE("\tint " <<  VISITED_ACCEPTING << ";");
}

void NFA::print_acyclic_impl (std::ostream &fout, const std::string &className)
{
	// for (auto i = 0u ; i < trans.size(); i++) {
	// 	PRINT_LINE("bool " << className << "::" << VISIT_PROC(i) << VISIT_PARAMS);
	// 	PRINT_LINE("{");
	// 	if (is_starting(i)) PRINT_LINE("\t++" << VISITED_ACCEPTING << ";");
	// 	PRINT_LINE("\t" << VISITED_IDX(i, "e") << " = true;");
	// 	for (const auto &n : trans_inv[i]) {
	// 		n.first.output_as_preds(fout, "e", "p");
	// 		PRINT_LINE("\t\tif (" << VISITED_IDX(n.second, "p") << ") {");
	// 		PRINT_LINE("\t\t\tif (" << VISITED_ACCEPTING << ") return false;");
	// 		PRINT_LINE("\t\t} else if (!" << VISIT_CALL(n.second, "p") << ") return false;");
	// 		PRINT_LINE("\t}");
	// 	}
	// 	if (is_starting(i)) PRINT_LINE("\t--" << VISITED_ACCEPTING << ";");
	// 	PRINT_LINE("\treturn true;");
	// 	PRINT_LINE("}");
	// 	PRINT_LINE("");
	// }

	// PRINT_LINE("bool " << className << "::" << IS_CONS);
	// PRINT_LINE("{");
	// PRINT_LINE("\t" << VISITED_ACCEPTING << " = 0;");
	// PRINT_LINE("\t" << VISITED_ARR << ".clear();");
	// PRINT_LINE("\t" << VISITED_ARR << ".resize(g.getMaxStamp() + 1);");
	// PRINT_LINE("\treturn true");
	// for (auto i = 0u ; i < trans.size(); i++) {
	// 	PRINT_LINE ("\t\t&& " << VISIT_CALL(i, "e")
	// 		    << (i + 1 == trans.size() ? ";" : ""));
	// }
	// PRINT_LINE("}");
	// PRINT_LINE("");
}
