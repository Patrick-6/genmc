#include <algorithm>
#include <fstream>
#include <iostream>
#include "NFA.hpp"

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

//-------------------------------------------------------------------

bool NFA::is_starting(int n) const
{
	return std::find(starting.begin(), starting.end(), n) != starting.end();
}

bool NFA::is_accepting(int n) const
{
	return std::find(accepting.cbegin(), accepting.cend(), n) != accepting.cend();
}

bool NFA::contains_edge (int n, NFA::tok_t tok, int m) const
{
	std::pair<tok_t, int>p {tok, m};
	return std::find(trans[n].cbegin(), trans[n].cend(), p) != trans[n].cend();
}

void NFA::add_edge (int n, tok_t tok, int m)
{
	if (contains_edge (n, tok, m)) return;
	trans[n].push_back({tok,m});
	trans_inv[m].push_back({tok,n});
}

void NFA::remove_edge (int n, tok_t tok, int m)
{
	auto it = std::remove(trans[n].begin(), trans[n].end(), std::pair<tok_t,int>(tok,m));
	trans[n].erase(it, trans[n].end());
	it = std::remove(trans_inv[m].begin(), trans_inv[m].end(), std::pair<tok_t,int>(tok,n));
	trans_inv[m].erase(it, trans_inv[m].end());
}

void NFA::add_outgoing_edges (int n, const tok_vector &edges)
{
	for (const auto &p : edges)
		add_edge (n, p.first, p.second);
}

void NFA::add_incoming_edges (int n, const tok_vector &edges)
{
	for (const auto &p : edges)
		add_edge (p.second, p.first, n);
}

// Remove node n and all its incoming and outgoing edges
void NFA::remove_node (int n)
{
//	std::cout << "Remove node " << n << " in " << *this;
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
//	std::cout << "End remove node" << *this << std::endl;
}

void NFA::flip()
{
	std::swap(starting, accepting);
	std::swap(trans, trans_inv);
}

// Check that starting nodes have no incoming edges
bool NFA::clean_starting() const
{
	for (int i = 0; i < trans_inv.size(); ++i)
		if (!trans_inv[i].empty() && is_starting(i))
			return false;
	return true;
}

// Ensure starting nodes have no incoming edges
void NFA::ensure_clean_starting()
{
	if (clean_starting())
		return;
	// Find starting node with no incoming edges
	int n = 0;
	while (n < trans.size()) {
		if (trans_inv[n].empty() && is_starting(n)) break;
		++n;
	}
	// If no such node exists, create one
	if (n == trans.size()) {
		trans.push_back({});
		trans_inv.push_back({});
	}
	// Add { (n,a,q) | (i,a,q) \in NFA, starting(i) \ {n} }
	for (int i = 0; i < trans.size(); ++i) {
		if (i == n || !is_starting(i)) continue;
		add_outgoing_edges(n, trans[i]);
	}
	starting = { n };
}

// Check that accepting nodes have no outgoing edges
bool NFA::clean_accepting() const
{
	for (int i = 0; i < trans.size(); ++i)
		if (!trans[i].empty() && is_accepting(i))
			return false;
	return true;
}

// Ensure all accepting nodes have no outgoing edges
void NFA::ensure_clean_accepting()
{
	flip();
	ensure_clean_starting();
	flip();
}

// Basic graph clean up
void NFA::simplify_basic ()
{
	bool changed;
	do {
		changed = false;
		// Remove duplicate edges
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

		// Compress [..] edges
		for(int i = trans.size() - 1; i >= 0; --i)
			for (int j = trans[i].size() - 1; j >= 0; --j) {
				auto p = trans[i][j];
				if (p.first[0] != '[') continue;
				if (is_accepting(p.second) && i != p.second) continue;
				std::cout << "Compacting edge " << i << " --" << p.first << "--> " << p.second << " in " << *this << std::endl;
				if (p.second != i) {
					for (const auto &q : trans[p.second]) {
						std::string g = "SEQ(" + p.first + "," + q.first + ")";
						add_edge (i, g, q.second);
					}
				}
				remove_edge (i, p.first, p.second);
				std::cout << "Result: " << *this << std::endl;
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

NFA NFA::make_singleton(const tok_t &c)
{
	NFA n;
	n.trans.push_back({{c, 1}});
	n.trans.push_back({});
	n.trans_inv.push_back({});
	n.trans_inv.push_back({{c, 0}});
	n.starting.insert(0);
	n.accepting.insert(1);
	return n;
}

void NFA::alt (const NFA &other)
{
	int n = trans.size();
	trans.resize(n + other.trans.size());
	trans_inv.resize(n + other.trans.size());
	for (const auto &k : other.starting) starting.insert (n + k);
	for (const auto &k : other.accepting) accepting.insert (n + k);
	for (int i = 0; i < other.trans.size(); ++i) {
		for (const auto &p : other.trans[i])
			add_edge (n + i, p.first, n + p.second);
	}
	simplify_basic();
}

void NFA::seq (const NFA &other)
{
	int n = trans.size();
	trans.resize(n + other.trans.size());
	trans_inv.resize(n + other.trans.size());
	for (int i = 0; i < other.trans.size(); ++i) {
		for (const auto &p : other.trans[i])
			add_edge (n + i, p.first, n + p.second);
	}
	for (const auto &a : accepting)
		for (const auto &b : other.starting)
			for (const auto &p : other.trans[b])
				add_edge(a, p.first, p.second + n);
	accepting.clear();
	for (const auto &k : other.accepting) accepting.insert (n + k);
	simplify_basic();
}

void NFA::plus ()
{
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

void NFA::all_suffixes ()
{
	for (int i = 0; i < trans.size(); ++i)
		starting.insert(i);
}

//-------------------------------------------------------------------

void NFA::simplify ()
{
	bool changed;
	do {
		simplify_basic();

		int s = trans.size();
		std::vector<bool> same (s * s);
		same.flip();

		do {
			changed = false;
			for (int i = 0; i < s; ++i)
				for (int j = 0; j < i; ++j) {
					if (!same[i * s + j]) continue;
					if (is_accepting(i) == is_accepting(j)
					    && std::all_of (trans[i].begin(), trans[i].end(), [&](const std::pair<std::string, int> &n) {
						return std::find_if(trans[j].begin(), trans[j].end(), [&](const std::pair<std::string, int> &m) {
							return n.first == m.first && same[n.second * s + m.second]; }) != trans[j].end(); })
					    && std::all_of (trans[j].begin(), trans[j].end(), [&](const std::pair<std::string, int> &n) {
						return std::find_if(trans[i].begin(), trans[i].end(), [&](const std::pair<std::string, int> &m) {
							return n.first == m.first && same[n.second * s + m.second]; }) != trans[i].end(); }))
						continue;
					same[i * s + j] = false;
					same[j * s + i] = false;
					changed = true;
				}
		} while (changed);
		changed = false;

		std::cout << "Current NFA: " << *this << std::endl;
		for (int i = trans.size() - 1; i >= 0 ; --i)
			for (int j = trans.size() - 1; j > i; --j)
				if (same[i * s + j]) {
					std::cout << "Nodes " << i << " and " << j << " are bisimilar." << std::endl;
					if (is_starting(j)) starting.insert(i);
					add_incoming_edges(i, trans_inv[j]);
					remove_node(j);
					changed = true;
				}
		std::cout << "Transformed NFA: " << *this << std::endl;
	} while (changed);
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

std::ostream & operator<< (std::ostream& ostr, const Token &t)
{
	if (!t.checks.empty())
		ostr << "[" << t.checks << "]";
	ostr << t.trans;
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

void printKaterNotice(const std::string &name, std::ostream &out = std::cout)
{
	out << "/* This file is generated automatically by Kater -- do not edit. */\n\n";
	return;
}

void NFA::print_visitors_header_file (const std::string &name)
{
	std::string className = std::string("KaterConsChecker") + name;

	std::ofstream fout (className + ".hpp");
	if (!fout.is_open()) {
		return;
	}

	printKaterNotice(name, fout);

	fout << "#ifndef __KATER_CONS_CHECKER_" << name << "_HPP__\n";
	fout << "#define __KATER_CONS_CHECKER_" << name << "_HPP__\n\n";

	fout << "#include \"ExecutionGraph.hpp\"\n";
	fout << "#include \"EventLabel.hpp\"\n";

	fout << "\nclass " << className << " {\nprivate:\n";
	fout << "\tconst ExecutionGraph & graph;\n";
	for (int i = 0 ; i < trans.size(); i++) {
		fout << "\tstd::vector<bool> visited" << i << ";\n";
	}

	for (int i = 0 ; i < trans.size(); i++) {
		fout << "\tbool visit" << i << "(const EventLabel &x);\n";
	}

	fout << "\npublic:\n";
	fout << "\t" << className << "(const ExecutionGraph &G) : graph(G)";
	for (int i = 0 ; i < trans.size(); i++) fout << ", visited" << i << "()";
	fout << " {}\n";
	fout << "\tbool isConsistent(const EventLabel &x);\n";
	fout << "};\n\n";

	fout << "#endif /* __KATER_CONS_CHECKER_" << className << "_HPP__ */\n";
}

void NFA::print_visitors_impl_file (const std::string &name)
{
	std::string className = std::string("KaterConsChecker") + name;

	std::ofstream fout (className + ".cpp");
	if (!fout.is_open()) {
		return;
	}

	printKaterNotice(name, fout);

	fout << "#include \"" << className << ".hpp\"\n";
	fout << "#include <vector>\n";

	for (int i = 0 ; i < trans.size(); i++) {
		fout << "\nbool " << className << "::visit" << i << "(const EventLabel &x)\n{\n";
		fout << "\tif (visited" << i << "[x.getStamp()]) return " << (is_accepting(i) ? "true" : "false") << ";\n";
		fout << "\tvisited[x.getStamp()] = true;\n";
		for (const auto &n : trans[i]) {
			fout << "\tfor (const auto &s : " << n.first << "_succs(G, x))\n";
			fout << "\t\tif(visit" << n.second << "(s)) return true;\n";
		}
		fout << "\treturn false;\n}\n";
	}

	/* I'm assuming that x has the largest timestamp in the graph.
	   If not, we have to read the largest timestamp from the graph
	   initialize the visited bit-vectors. */
	fout << "\nbool " << className << "::isConsistent(const EventLabel &x)\n{\n";
	for (int i = 0 ; i < trans.size(); i++) {
		fout << "\tvisited" << i << ".clear();\n";
		fout << "\tvisited" << i << ".resize(x.getStamp() + 1);\n";
	}
	fout << "\treturn true";
	for (auto i : starting) fout << " && visit" << i << "(x)";
	fout << ";\n}\n";
}
