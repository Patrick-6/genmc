#include "Config.hpp"
#include "NFA.hpp"
#include <algorithm>
#include <map>
#include <fstream>
#include <iostream>

template<typename T>
static std::ostream & operator<< (std::ostream& ostr, const std::set<T> &s);

using my_pair = std::pair<TransLabel,int>;

static bool has_only_self_loops(const NFA::trans_t &trans, int n)
{
	for (auto &it : trans[n])
		if (it.second != n)
			return false;
	return true;
}

static void sort_edges (NFA::trans_t &trans)
{
	for (auto &it : trans) {
		std::sort(it.begin(), it.end());
	}
}

static void removal_remap (NFA::trans_t &trans, int n)
{
	for(auto &it : trans)
		for (int j = it.size() - 1; j >= 0; --j)
			if (it[j].second == n) it.erase(it.begin() + j);
			else if (it[j].second >= n) it[j].second = it[j].second - 1;
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

//-------------------------------------------------------------------

bool NFA::is_starting(int n) const
{
	return std::find(starting.begin(), starting.end(), n) != starting.end();
}

bool NFA::is_accepting(int n) const
{
	return std::find(accepting.cbegin(), accepting.cend(), n) != accepting.cend();
}

bool NFA::contains_edge (int n, const TransLabel &l, int m) const
{
	my_pair p {l, m};
	return std::find(trans[n].cbegin(), trans[n].cend(), p) != trans[n].cend();
}

void NFA::add_edge (int n, const TransLabel &l, int m)
{
	if (contains_edge (n, l, m)) return;
	trans[n].push_back({l,m});
	my_pair p {l,n};
	p.first.flip();
	trans_inv[m].push_back(p);
}

void NFA::remove_edge (int n, const TransLabel &l, int m)
{
	my_pair p {l,m};
	auto it = std::remove(trans[n].begin(), trans[n].end(), p);
	trans[n].erase(it, trans[n].end());
	p.first.flip();
	p.second = n;
	it = std::remove(trans_inv[m].begin(), trans_inv[m].end(), p);
	trans_inv[m].erase(it, trans_inv[m].end());
}

void NFA::add_outgoing_edges (int n, const tok_vector &edges)
{
	for (const auto &p : edges)
		add_edge (n, p.first, p.second);
}

void NFA::add_incoming_edges (int n, const tok_vector &edges)
{
	for (auto p : edges) {
		p.first.flip();
		add_edge (p.second, p.first, n);
	}
}

// Remove node n and all its incoming and outgoing edges
void NFA::remove_node (int n)
{
	removal_remap (trans, n);
	removal_remap (trans_inv, n);
	trans.erase(trans.begin() + n);
	trans_inv.erase(trans_inv.begin() + n);
	std::set<int> s;
	for (auto i : starting) {
		if (i < n) s.insert(i);
		if (i > n) s.insert(i - 1);
	}
	std::swap (starting, s);
	s.clear();
	for (auto i : accepting) {
		if (i < n) s.insert(i);
		if (i > n) s.insert(i - 1);
	}
	std::swap (accepting, s);
}

void NFA::flip()
{
	std::swap(starting, accepting);
	std::swap(trans, trans_inv);
}

// Basic graph clean up
void NFA::simplify_basic ()
{
	bool changed;
	do {
		changed = false;
		// Keep the edges sorted
		sort_edges(trans);
		sort_edges(trans_inv);

		// Erase non-accepting nodes with no outgoing edges
		// and non-starting nodes with no incoming edges
		for(int i = trans.size() - 1; i >= 0; --i)
			if ((has_only_self_loops (trans, i) && !is_accepting(i))
			    || (has_only_self_loops (trans_inv, i) && !is_starting(i))) {
				remove_node (i);
				changed = true;
			}

		// Erase duplicate nodes with same outgoing edges
		for(int i = trans.size() - 1; i >= 0; --i)
			for(int j = trans.size() - 1; j > i; --j) {
				if (is_accepting(i) != is_accepting(j)) continue;
				if (trans[i] != trans[j]) continue;
				if (is_starting(j)) starting.insert(i);
				add_incoming_edges (i, trans_inv[j]);
				remove_node (j);
				changed = true;
			}

		// Erase duplicate nodes with same incoming edges
		for(int i = trans.size() - 1; i >= 0; --i)
			for(int j = trans.size() - 1; j > i; --j) {
				if (is_starting(i) != is_starting(j)) continue;
				if (trans_inv[i] != trans_inv[j]) continue;
				if (is_accepting(j)) accepting.insert(i);
				add_outgoing_edges (i, trans[j]);
				remove_node (j);
				changed = true;
			}

	} while (changed);
}

//-------------------------------------------------------------------

NFA NFA::make_empty()
{
	NFA n;
	n.trans.push_back({});
	n.trans_inv.push_back({});
	n.starting.insert(0);
	n.accepting.insert(0);
	return n;
}

NFA NFA::make_singleton(const TransLabel &c)
{
	NFA n;
	TransLabel flipc = c;
	flipc.flip();
	n.trans.push_back({{c, 1}});
	n.trans.push_back({});
	n.trans_inv.push_back({});
	n.trans_inv.push_back({{flipc, 0}});
	n.starting.insert(0);
	n.accepting.insert(1);
	return n;
}

void NFA::alt (const NFA &other)
{
	int n = trans.size();

	// Append states and transitions of `other`
	trans.resize(n + other.trans.size());
	trans_inv.resize(n + other.trans.size());
	for (int i = 0; i < other.trans.size(); ++i) {
		for (const auto &p : other.trans[i])
			add_edge (n + i, p.first, n + p.second);
	}

	// Take the union of starting and accepting states
	for (const auto &k : other.starting) starting.insert (n + k);
	for (const auto &k : other.accepting) accepting.insert (n + k);
}

void NFA::seq (const NFA &other)
{
	int n = trans.size();

	// Append states and transitions of `other`
	trans.resize(n + other.trans.size());
	trans_inv.resize(n + other.trans.size());
	for (int i = 0; i < other.trans.size(); ++i) {
		for (const auto &p : other.trans[i])
			add_edge (n + i, p.first, n + p.second);
	}

	// Add transitions `this->accepting --> other.starting`
	for (const auto &a : accepting)
		for (const auto &b : other.starting)
			for (const auto &p : other.trans[b])
				add_edge(a, p.first, p.second + n);

	// Calculate accepting states
	if (std::none_of(other.starting.begin(), other.starting.end(),
			 [&](int i) { return other.is_accepting(i); }))
		accepting.clear();
	for (const auto &k : other.accepting) accepting.insert (n + k);
}

void NFA::plus ()
{
	// Add a transition from every accepting states
	// to every successor of a starting state
	for (const auto &a : accepting)
		for (const auto &s : starting)
			add_outgoing_edges(a, trans[s]);
}

void NFA::or_empty ()
{
	// Does the NFA already accept the empty string?
	// If so, then exit.
	for (const auto &s : starting)
		if (is_accepting (s))
			return;
	// Otherwise, find starting node with no incoming edges
	int n = 0;
	while (n < trans.size()) {
		if (trans_inv[n].empty() && is_starting(n)) break;
		++n;
	}
	// If no such node exists, create one
	if (n == trans.size()) {
		trans.push_back({});
		trans_inv.push_back({});
		starting.insert(n);
	}
	// Make that starting node accepting
	accepting.insert(n);
}

void NFA::star ()
{
	plus();
	or_empty();
}

// Convert to a deterministic automaton using the subset construction
std::pair<NFA, std::vector<std::set<int>>> NFA::to_DFA() const
{
	NFA res;
	std::map<std::set<int>, int> m;
	std::vector<std::set<int>> v;
	// Starting state of subset automaton
	v.push_back(starting);
	m.insert({starting, 0});
	res.trans.push_back({});
	res.trans_inv.push_back({});
	res.starting.insert(0);
	/////////////////////////////////////////////////
	for (int i = 0; i < v.size(); i++) {
		// For all outgoing transitions from state v[i]
		auto ss = v[i];
		for (const auto &s : ss)
			for (const auto &p : trans[s]) {
				// Calculate next set
				std::set<int> next;
				for (const auto &s2 : v[i])
					for (const auto &p2 : trans[s2])
						if (p2.first == p.first) next.insert(p2.second);
				// Find index of next set
				auto it = m.find(next);
				int n = (it == m.end())
					  ? (m.insert({next, v.size()}),
					     res.trans.push_back({}),
					     res.trans_inv.push_back({}),
					     v.push_back(next), v.size() - 1)
					  : it->second;
				// Add edge i --p.first--> n
				res.add_edge(i, p.first, n);
			}
	}
	// Calculate accepting states
	for (int i = 0; i < v.size(); i++)
		for (const auto &s : v[i])
			if (is_accepting(s)) res.accepting.insert(i);
	return {res, v};
}


// Return the state composition matrix, which is useful for minimizing the
// states of an NFA.  See Definition 3 of Kameda and Weiner: On the State
// Minimization of Nondeterministic Finite Automata
std::vector<std::vector<char>> NFA::get_state_composition_matrix ()
{
	flip();
	auto p = to_DFA();
	flip();
	if (config.verbose > 1) std::cout << "State composition matrix: " << std::endl;
	std::vector<std::vector<char>> v;
	int vsize = p.second.size();
	for (int i = 0; i < trans.size(); ++i) {
		auto bv = std::vector<char>(vsize);
		for (int j = 0; j < vsize; ++j)
			if (p.second[j].find(i) != p.second[j].end())
				bv[j] = 1;
		v.push_back(bv);
		if (config.verbose > 1) std::cout << bv << ": " << i << std::endl;
	}
	return v;
}


// Reduce the NFA using the state composition matrix (cf. Kameda and Weiner)
void NFA::scm_reduce ()
{
	simplify_basic();
	auto v = get_state_composition_matrix ();
	int vsize = v[0].size();
	for (int i = trans.size() - 1; i >= 0; --i) {
		// Is v[i] the equal to the union of some other rows?
		auto bv = std::vector<char>(vsize);
		for (int j = 0; j < trans.size(); ++j) {
			if (j == i) continue;
			if (!is_subset(v[j], v[i])) continue;
			take_union(bv, v[j]);
		}
		if (bv != v[i]) continue;
		if (config.verbose > 1) std::cout << "erase node " << i << " with";
		// If so, we can remove v[i].
		for (int j = 0; j < trans.size(); ++j) {
			if (j == i) continue;
			if (!is_subset(v[j], v[i])) continue;
			if (config.verbose > 1) std::cout << " " << j;
			add_incoming_edges(j, trans_inv[i]);
		}
		if (config.verbose > 1) std::cout << std::endl;
		remove_node(i);
		v.erase(v.begin() + i);
	}
}


void NFA::compact_edges()
{
	// Join `[...]` edges with successor edges
	for(int i = trans.size() - 1; i >= 0; --i)
		for (int j = trans[i].size() - 1; j >= 0; --j) {
			auto p = trans[i][j];
			if (!p.first.is_empty_trans()) continue;
			if (is_accepting(p.second) && i != p.second) continue;
			if (config.verbose > 1)
				std::cout << "Compacting edge " << i << " --"
					  << p.first << "--> " << p.second << std::endl;
			if (p.second != i) {
				for (const auto &q : trans[p.second]) {
					TransLabel l = p.first.seq(q.first);
					if (l.is_valid())
						add_edge (i, l, q.second);
				}
			}
			remove_edge (i, p.first, p.second);
		}
	// Remove redundant self loops
	for(int i = trans.size() - 1; i >= 0; --i)
		for (int j = trans[i].size() - 1; j >= 0; --j) {
			auto p = trans[i][j];
			if (p.second == i) continue;
			if (!std::all_of (trans[i].begin(), trans[i].end(), [&](const my_pair &q) {
				return q.first == p.first && (q.second == i || std::find(trans[p.second].begin(), trans[p.second].end(), q) != trans[p.second].end()); }))
				continue;
			remove_edge (i, p.first, i);
		}
}


void NFA::simplify ()
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
}

void NFA::simplifyReduce()
{
	// We have to find the transitive reduction, so take the
	// reflexive-transitive closure to condense the NFA
	star();
	simplify();
	// and if the condensed NFA containg only one starting and only one
	// accepting node (which ought to be identical to the starting node),
	// then make sure no incoming edges to the starting node are used.
	if (accepting.size() == 1 && starting == accepting) {
		int n = *accepting.begin(); // the unique starting/accepting state
		int m = trans.size();       // the new starting state
		trans.push_back({});
		trans_inv.push_back({});
		starting.clear();
		starting.insert(m);
		add_outgoing_edges(m, trans[n]);
		for (int j = trans[n].size() - 1; j >= 0; --j)
			remove_edge(n, trans[n][j].first, trans[n][j].second);
		simplify();
	}
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

std::ostream & operator<< (std::ostream& ostr, const NFA& nfa)
{
	ostr << "[NFA with " << nfa.trans.size() << " states]" << std::endl;
	ostr << "starting: {" << nfa.starting << "} accepting: {" << nfa.accepting << "}" << std::endl;
	for (int i = 0 ; i < nfa.trans.size(); i++) {
		for (const auto &n : nfa.trans[i])
			ostr << "\t" << i << " --" << n.first << "--> " << n.second << std::endl;
	}
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
	for (auto i = 0u ; i < trans.size(); i++)
		PRINT_LINE("\tvoid " << VISIT_PROC(i) << VISIT_PARAMS);

	PRINT_LINE("\tstd::vector<std::bitset<" << trans.size() <<  "> > " << VISITED_ARR << ";");
}

void NFA::printCalculatorImplHelper(std::ostream &fout, const std::string &className,
				    int whichCalc, bool reduce)
{
	for (auto i = 0u ; i < trans.size(); i++) {
		PRINT_LINE("void " << className << "::" << VISIT_PROC(i) << VISIT_PARAMS);
		PRINT_LINE("{");
		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("");

		PRINT_LINE("\t" << VISITED_IDX(i,"e") << " = true;");
		if (is_starting (i)) {
			PRINT_LINE("\tcalcRes.insert(e);");
			if (reduce) {
				PRINT_LINE("\tfor (const auto &p : calc" << whichCalc << "_preds(g, e)) {");
				PRINT_LINE("\t\tcalcRes.erase(p);");
				for (int j : accepting)
					PRINT_LINE("\t\t" << VISITED_IDX(j, "p") << " = true;");
				PRINT_LINE("\t}");
			}
		}
		for (const auto &n : trans_inv[i]) {
			n.first.output_as_preds(fout, "e", "p");
			PRINT_LINE("\t\tif (" << VISITED_IDX(n.second, "p") << ") continue;");
			PRINT_LINE("\t\t" << VISIT_CALL(n.second, "p"));
			PRINT_LINE("\t}");
		}
		PRINT_LINE("}");
		PRINT_LINE("");
	}

	PRINT_LINE(VSET << " " << className << "::" << CALC);
	PRINT_LINE("{");
	PRINT_LINE("\t" << VSET << " calcRes;");
	PRINT_LINE("\t" << VISITED_ARR << ".clear();");
	PRINT_LINE("\t" << VISITED_ARR << ".resize(g.getMaxStamp() + 1);");
	for (auto &i : accepting)
		PRINT_LINE("\t" << VISIT_CALL(i, "e"));
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
	for (auto i = 0u ; i < trans.size(); i++)
		PRINT_LINE("\tbool " << VISIT_PROC(i) << VISIT_PARAMS << ";");

	PRINT_LINE("\tstd::vector<std::bitset< " << trans.size() <<  "> > " << VISITED_ARR << ";");
	PRINT_LINE("\tint " <<  VISITED_ACCEPTING << ";");
}

void NFA::print_acyclic_impl (std::ostream &fout, const std::string &className)
{
	for (auto i = 0u ; i < trans.size(); i++) {
		PRINT_LINE("bool " << className << "::" << VISIT_PROC(i) << VISIT_PARAMS);
		PRINT_LINE("{");
		if (is_starting(i)) PRINT_LINE("\t++" << VISITED_ACCEPTING << ";");
		PRINT_LINE("\t" << VISITED_IDX(i, "e") << " = true;");
		for (const auto &n : trans_inv[i]) {
			n.first.output_as_preds(fout, "e", "p");
			PRINT_LINE("\t\tif (" << VISITED_IDX(n.second, "p") << ") {");
			PRINT_LINE("\t\t\tif (" << VISITED_ACCEPTING << ") return false;");
			PRINT_LINE("\t\t} else if (!" << VISIT_CALL(n.second, "p") << ") return false;");
			PRINT_LINE("\t}");
		}
		if (is_starting(i)) PRINT_LINE("\t--" << VISITED_ACCEPTING << ";");
		PRINT_LINE("\treturn true;");
		PRINT_LINE("}");
		PRINT_LINE("");
	}

	PRINT_LINE("bool " << className << "::" << IS_CONS);
	PRINT_LINE("{");
	PRINT_LINE("\t" << VISITED_ACCEPTING << " = 0;");
	PRINT_LINE("\t" << VISITED_ARR << ".clear();");
	PRINT_LINE("\t" << VISITED_ARR << ".resize(g.getMaxStamp() + 1);");
	PRINT_LINE("\treturn true");
	for (auto i = 0u ; i < trans.size(); i++) {
		PRINT_LINE ("\t\t&& " << VISIT_CALL(i, "e")
			    << (i + 1 == trans.size() ? ";" : ""));
	}
	PRINT_LINE("}");
	PRINT_LINE("");
}
