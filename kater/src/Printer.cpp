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
#include "Printer.hpp"

std::vector<unsigned> Printer::calcToIdxMap;

const char *genmcCopyright =
R"(/*
 * GenMC -- Generic Model Checking.
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
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */
)";

const char *katerNotice =
R"(/*******************************************************************************
 * CAUTION: This file is generated automatically by Kater -- DO NOT EDIT.
 *******************************************************************************/
)";

const std::unordered_map<Relation::Builtin, Printer::RelationOut> Printer::relationNames = {
        /* po */
        {Relation::po_imm,	{"po_imm_succs",     "po_imm_preds"}},
        {Relation::po_loc_imm,	{"poloc_imm_succs",  "poloc_imm_preds"}},
	/* deps */
        {Relation::ctrl_imm,	{"?",                "ctrl_preds"}},
        {Relation::addr_imm,	{"?",                "addr_preds"}},
        {Relation::data_imm,	{"?",                "data_preds"}},
	/* same thread */
	{Relation::same_thread,	{ "same_thread",     "same_thread"}},
	/* same location */
        {Relation::alloc,		{ "alloc_succs",     "alloc"}},
        {Relation::frees,		{ "frees",           "alloc"}},
        {Relation::loc_overlap,	{ "?",               "loc_preds"}},
	/* rf, co, fr, detour */
        {Relation::rf,		{ "rf_succs",        "rf_preds"}},
        {Relation::rfe,		{ "rfe_succs",       "rfe_preds"}},
        {Relation::rfi,		{ "rfi_succs",       "rfi_preds"}},
        {Relation::tc,		{ "tc_succs",        "tc_preds"}},
        {Relation::tj,		{ "tj_succs",        "tj_preds"}},
        {Relation::mo_imm,	{ "co_imm_succs",    "co_imm_preds"}},
        {Relation::moe,		{ "co_imm_succs",    "co_imm_preds"}},
        {Relation::moi,		{ "co_imm_succs",    "co_imm_preds"}},
        {Relation::fr_imm,	{ "fr_imm_succs",    "fr_imm_preds"}},
        {Relation::fre,		{ "fr_imm_succs",    "fr_imm_preds"}},
        {Relation::fri,		{ "fr_imm_succs",    "fr_imm_preds"}},
        {Relation::detour,	{ "detour_succs",    "detour_preds"}},
};

void openFileForWriting(const std::string &filename, std::ofstream &fout)
{
	fout.open(filename, std::ios_base::out);
	if (!fout.is_open()) {
		std::cerr << "Could not open file for writing: " << filename << "\n";
		exit(EPRINT);
	}
}

Printer::Printer(const std::string &dirPrefix, const std::string &outPrefix)
{
	auto name = outPrefix != "" ? outPrefix : "Demo";

	/* Construct all the names to be used */
	std::transform(name.begin(), name.end(), std::back_inserter(prefix), ::toupper);
	className = prefix + "Checker";
	guardName = std::string("__") + prefix + "_CHECKER_HPP__";

	/* Open required streams */
	if (outPrefix != "") {
		openFileForWriting(dirPrefix + "/" + className + ".hpp", foutHpp);
		outHpp = &foutHpp;
		openFileForWriting(dirPrefix + "/" + className + ".cpp", foutCpp);
		outCpp = &foutCpp;
	}
}

std::size_t replaceAll(std::string& inout, const std::string &what, const std::string &with)
{
	std::size_t count{};
	for (std::string::size_type pos{};
	     inout.npos != (pos = inout.find(what.data(), pos, what.length()));
	     pos += with.length(), ++count) {
		inout.replace(pos, what.length(), with.data(), with.length());
	}
	return count;
}

void Printer::printPredSet(std::ostream &ostr, const std::string &arg, const PredicateSet &preds)
{
	auto s = preds.toGenmcString();
	if (s == "")
		return;
	replaceAll(s, "#", arg);
	ostr << "if (" << s << ")";
}

void Printer::printRelation(std::ostream& ostr, const std::string &res,
			    const std::string &arg, const TransLabel *r)
{
	if (r->isPredicate()) {
		ostr << "if (auto " << res << " = " << arg << "->getPos(); true)";
		return;
	}

	if (r->isBuiltin()) {
		const auto &outs = relationNames.find(r->getRelation()->toBuiltin())->second;
		const auto &s = r->getRelation()->isInverse() ? outs.pred : outs.succ;
		// if ((n.type == RelType::OneOne) || (flipped && n.type == RelType::ManyOne))
		// 	ostr << "\tif (auto " << res << " = " << s << ") {\n";
		// else
		ostr << "for (auto &" << res << " : " << s << "(g, " << arg << "->getPos()))";
		return;
	}

	auto index = getCalcIdx(r->getCalcIndex());
	if (viewCalcs.count(r->getCalcIndex())) {
		ostr << "for (auto &" << res << " : maximals(" << arg << "->view(" << index << ")))";
		return;
	}
	ostr << "for (auto &" << res << " : " << arg << "->calculated(" << index << "))";
	return;
}

void Printer::printTransLabel(const TransLabel *t, const std::string &res, const std::string &arg)
{
	printPredSet(cpp(), arg, t->getPreChecks());
	printRelation(cpp(), res, arg, t);
	printPredSet(cpp(), "g.getEventLabel(" + res + ")", t->getPostChecks());
}

void Printer::printHppHeader()
{
	hpp() << genmcCopyright << "\n"
	      << katerNotice << "\n";

	hpp() << "#ifndef " << guardName << "\n"
	      << "#define " << guardName << "\n"
	      << "\n"
	      << "#include \"config.h\"\n"
	      << "#include \"ExecutionGraph.hpp\"\n"
	      << "#include \"GraphIterators.hpp\"\n"
	      << "#include \"MaximalIterator.hpp\"\n"
	      << "#include \"PersistencyChecker.hpp\"\n"
	      << "#include \"VSet.hpp\"\n"
	      << "#include <cstdint>\n"
	      << "#include <vector>\n"
	      << "\n"
	      << "class " << className << " {\n"
	      << "\n"
	      << "private:\n"
	      << "\tenum class NodeStatus : unsigned char { unseen, entered, left };\n"
	      << "\n"
	      << "\tstruct NodeCountStatus {\n"
	      << "\t\tNodeCountStatus() = default;\n"
	      << "\t\tNodeCountStatus(uint16_t c, NodeStatus s) : count(c), status(s) {}\n"
	      << "\t\tuint16_t count = 0;\n"
	      << "\t\tNodeStatus status = NodeStatus::unseen;\n"
	      << "\t};\n"
	      << "\n"
	      << "public:\n"
	      << "\t" << className << "(ExecutionGraph &g) : g(g) {}\n"
	      << "\n"
	      << "\tstd::vector<VSet<Event>> calculateSaved(const Event &e);\n"
	      << "\tstd::vector<View> calculateViews(const Event &e);\n"
	      << "\tbool isConsistent(const Event &e);\n"
	      << "\tbool isRecoveryValid(const Event &e);\n"
	      << "\tstd::unique_ptr<VectorClock> getPPoRfBefore(const Event &e);\n"
	      << "\n"
	      << "private:\n";
}

void Printer::printCppHeader()
{
	cpp() << genmcCopyright << "\n"
	      << katerNotice << "\n";

	cpp() <<  "#include \"" << className << ".hpp\"\n"
	      << "\n";
}

void Printer::printHppFooter()
{
	hpp() << "\tstd::vector<VSet<Event>> saved;\n"
	      << "\tstd::vector<View> views;\n"
	      << "\n"
	      << "\tExecutionGraph &g;\n"
	      << "\n"
	      << "\tExecutionGraph &getGraph() { return g; }\n"
	      << "};\n"
	      << "\n"
	      << "#endif /* " << guardName << " */\n";
}

void Printer::printCppFooter()
{
}

void Printer::outputHpp(const CNFAs &cnfas)
{
	printHppHeader();

	auto i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		printCalculatorHpp(nfaStatus.first, i++, nfaStatus.second);
	});

	i = 0u;
	std::for_each(cnfas.incl_begin(), cnfas.incl_end(), [&](auto &nfaPair){
		printInclusionHpp(nfaPair.lhs, nfaPair.rhs, i++);
	});

	printAcyclicHpp(cnfas.getAcyclic());

	printRecoveryHpp(cnfas.getRecovery());

	printPPoRfHpp(cnfas.getPPoRf().first, cnfas.getPPoRf().second);

	printHppFooter();
}

void Printer::outputCpp(const CNFAs &cnfas)
{
	printCppHeader();

	/* Print calculators + calculateSaved() + calculateViews()*/
	auto i = 0u;
	auto vc = 0u;
	auto sc = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		calcToIdxMap.push_back((nfaStatus.second == VarStatus::View) ? vc++ : sc++);
		if (nfaStatus.second == VarStatus::View)
			viewCalcs.insert(i);
		printCalculatorCpp(nfaStatus.first, i++, nfaStatus.second);
	});
	cpp() << "std::vector<VSet<Event>> " << className << "::calculateSaved(const Event &e)\n"
	      << "{\n";
	i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		if (nfaStatus.second != VarStatus::View)
			cpp() << "\tsaved.push_back(calculate" << i << "(e));\n";
		++i;
	});
	cpp() << "\treturn std::move(saved);\n"
	      << "}\n"
	      << "\n";
	cpp() << "std::vector<View> " << className << "::calculateViews(const Event &e)\n"
	      << "{\n";
	i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		if (nfaStatus.second == VarStatus::View)
			cpp() << "\tviews.push_back(calculate" << i << "(e));\n";
		++i;
	});
	cpp() << "\treturn std::move(views);\n"
	      << "}\n"
	      << "\n";

	i = 0u;
	std::for_each(cnfas.incl_begin(), cnfas.incl_end(), [&](auto &nfaPair){
		printInclusionCpp(nfaPair.lhs, nfaPair.rhs, i++);
	});

	/* Print acyclicity routines + isConsistent() */
	printAcyclicCpp(cnfas.getAcyclic());
	cpp() << "bool " << className << "::isConsistent(const Event &e)\n"
	      << "{\n"
	      << "\treturn isAcyclic(e);\n"
	      << "}\n"
	      << "\n";

	printRecoveryCpp(cnfas.getRecovery());
	cpp() << "bool " << className << "::isRecoveryValid(const Event &e)\n"
	      << "{\n"
	      << "\treturn isRecAcyclic(e);\n"
	      << "}\n"
	      << "\n";

	/* pporf-before getter */
	printPPoRfCpp(cnfas.getPPoRf().first, cnfas.getPPoRf().second);
	cpp() << "std::unique_ptr<VectorClock> " << className << "::getPPoRfBefore(const Event &e)\n"
	      << "{\n"
	      << "\treturn LLVM_MAKE_UNIQUE<" << (cnfas.getPPoRf().second ? "DepView" : "View")
	      				      << ">(calcPPoRfBefore(e));\n"
	      << "}\n"
	      << "\n";

	printCppFooter();
}

void Printer::output(const CNFAs &cnfas)
{
	outputHpp(cnfas);
	outputCpp(cnfas);
}

template<typename ITER>
std::unordered_map<NFA::State *, unsigned> assignStateIDs(ITER &&begin, ITER &&end)
{
	std::unordered_map<NFA::State *, unsigned> result;

	auto i = 0u;
	std::for_each(begin, end, [&](auto &s){ result[&*s] = i++; });
	return result;
}

#define GET_ID(nfa_id, state_id) nfa_id << "_" << state_id

void Printer::printAcyclicHpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitAcyclicX() for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tbool visitAcyclic" << ids[&*s] << "(const Event &e)" << ";\n";
	});
	hpp() << "\n";

	/* isAcyclic() for the automaton */
	hpp() << "\tbool isAcyclic(const Event &e)" << ";\n"
	      << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeCountStatus> visitedAcyclic" << ids[&*s] << ";\n";
	});
	hpp() << "\n";

	/* accepting counter */
	hpp() << "\tuint16_t visitedAccepting = 0;\n";
}

void Printer::printAcyclicCpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* Print a "visitAcyclicXX" for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "bool " << className << "::visitAcyclic" << ids[&*s] << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(" << "e" << ");\n"
		      << "\tauto t = 0u;\n"
		      << "\n";

		if (nfa.isStarting(&*s))
			cpp() << "\t++visitedAccepting;\n";
		cpp() << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp()] = "
								"{ visitedAccepting, NodeStatus::entered };\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto &node = visitedAcyclic" << ids[t.dest] << "[g.getEventLabel(p)->getStamp()];\n"
			      << "\t\tif (node.status == NodeStatus::unseen && !visitAcyclic" << ids[t.dest] << "(p))\n"
			      << "\t\t\treturn false;\n"
			      << "\t\telse if (node.status == NodeStatus::entered && visitedAccepting > node.count)\n"
			      << "\t\t\treturn false;\n"
			      <<"\t}\n";
		});
		if (nfa.isStarting(&*s))
			cpp() << "\t--visitedAccepting;\n";
		cpp() << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp()] = "
			"{ visitedAccepting, NodeStatus::left };\n"
		      << "\treturn true;\n"
		      << "}\n"
		      << "\n";
	});

	/* Print a "isAcyclicX" for the automaton */
	cpp() << "bool " << className << "::isAcyclic(const Event &e)\n"
	      << "{\n"
	      << "\tvisitedAccepting = 0;\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedAcyclic" << ids[&*s] << ".clear();\n"
		      << "\tvisitedAcyclic" << ids[&*s] << ".resize(g.getMaxStamp() + 1);\n";
	});
	cpp() << "\treturn true";
	for (auto sIt = nfa.states_begin(), sE = nfa.states_end(); sIt != sE; ++sIt) {
		if (std::all_of((*sIt)->out_begin(), (*sIt)->out_end(), [&](auto &t){
			auto rOpt = t.label.getRelation();
			return rOpt.has_value() && !rOpt->isInverse() &&
				(Relation::createBuiltin(Relation::Builtin::po_imm).includes(*rOpt) ||
				 Relation::createBuiltin(Relation::Builtin::rf).includes(*rOpt) ||
				 !rOpt->isBuiltin());
				}))
			continue;
		cpp() << "\n\t\t&& visitAcyclic" << ids[&**sIt] << "(e)";
	}
	cpp() << ";\n"
	      << "}\n"
	      << "\n";
}

void Printer::printRecoveryHpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitRecoveryX() for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tbool visitRecovery" << ids[&*s] << "(const Event &e)" << ";\n";
	});
	hpp() << "\n";

	/* isRecAcyclic() for the automaton */
	hpp() << "\tbool isRecAcyclic(const Event &e)" << ";\n"
	      << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeCountStatus> visitedRecovery" << ids[&*s] << ";\n";
	});
	hpp() << "\n";

	/* accepting counter */
	hpp() << "\tuint16_t visitedRecAccepting = 0;\n";
}

void Printer::printRecoveryCpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* Print a "visitRecoveryXX" for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "bool " << className << "::visitRecovery" << ids[&*s] << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(" << "e" << ");\n"
		      << "\tauto t = 0u;\n"
		      << "\n";

		if (nfa.isStarting(&*s))
			cpp() << "\t++visitedRecAccepting;\n";
		cpp() << "\tvisitedRecovery" << ids[&*s] << "[lab->getStamp()] = "
								"{ visitedRecAccepting, NodeStatus::entered };\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto &node = visitedRecovery" << ids[t.dest] << "[g.getEventLabel(p)->getStamp()];\n"
			      << "\t\tif (node.status == NodeStatus::unseen && !visitRecovery" << ids[t.dest] << "(p))\n"
			      << "\t\t\treturn false;\n"
			      << "\t\telse if (node.status == NodeStatus::entered && visitedRecAccepting > node.count)\n"
			      << "\t\t\treturn false;\n"
			      <<"\t}\n";
		});
		if (nfa.isStarting(&*s))
			cpp() << "\t--visitedRecAccepting;\n";
		cpp() << "\tvisitedRecovery" << ids[&*s] << "[lab->getStamp()] = "
			"{ visitedRecAccepting, NodeStatus::left };\n"
		      << "\treturn true;\n"
		      << "}\n"
		      << "\n";
	});

	/* Print a "isRecAcyclicX" for the automaton */
	cpp() << "bool " << className << "::isRecAcyclic(const Event &e)\n"
	      << "{\n"
	      << "\tvisitedRecAccepting = 0;\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedRecovery" << ids[&*s] << ".clear();\n"
		      << "\tvisitedRecovery" << ids[&*s] << ".resize(g.getMaxStamp() + 1);\n";
	});
	cpp() << "\treturn true";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\n\t\t&& visitRecovery" << ids[&*s] << "(e)";
	});
	cpp() << ";\n"
	      << "}\n"
	      << "\n";
}

void Printer::printCalculatorHpp(const NFA &nfa, unsigned id, VarStatus status)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitCalcXX for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tvoid visitCalc" << GET_ID(id, ids[&*s]) << "(const Event &e, "
		      << ((status == VarStatus::View) ? "View &" : "VSet<Event> &") << "calcRes);\n";
	});
	hpp() << "\n";

	/* calculateX for the automaton */
	hpp() << "\t" << ((status == VarStatus::View) ? "View" : "VSet<Event>") << " calculate" << id << "(const Event &e);\n";
	hpp() << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeStatus> visitedCalc" << GET_ID(id, ids[&*s]) << ";\n";
	});
	hpp() << "\n";
}

void Printer::printCalculatorCpp(const NFA &nfa, unsigned id, VarStatus status)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "void " << className << "::visitCalc" << GET_ID(id, ids[&*s]) << "(const Event &e, "
		      << ((status == VarStatus::View) ? "View &" : "VSet<Event> &") << "calcRes)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(e);\n"
		      << "\tauto t = 0u;\n"
		      << "\n"
		      << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::entered;\n";
		if (nfa.isStarting(&*s)) {
			if (status != VarStatus::View)
				cpp() << "\tcalcRes.insert(e);\n";
			if (status == VarStatus::Reduce) {
				cpp() << "\tfor (const auto &p : lab->calculated(" << getCalcIdx(id) << ")) {\n"
				      << "\t\tcalcRes.erase(p);\n";
				std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &a){
					cpp() << "\t\tvisitedCalc" << GET_ID(id, ids[a]) << "[g.getEventLabel(p)->getStamp()] = NodeStatus::left;\n";
				});
				cpp() << "\t}\n";
			} else if (status == VarStatus::View) {
				cpp() << "\tcalcRes.update(lab->view(" << getCalcIdx(id) << "));\n"
				      << "\tcalcRes.updateIdx(lab->getPos());\n";
			}
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto status = visitedCalc" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()];\n"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitCalc" << GET_ID(id, ids[t.dest]) << "(p, calcRes);\n"
			      <<"\t}\n";
		});
		cpp() << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::left;\n"
		      << "}\n"
		      << "\n";
	});

	cpp() << ((status == VarStatus::View) ? "View " : "VSet<Event> ") << className << "::calculate" << id << "(const Event &e)\n"
	      << "{\n"
	      << "\t" << ((status == VarStatus::View) ? "View" : "VSet<Event>") << " calcRes;\n";
	if (status == VarStatus::View)
		cpp() << "calcRes.updateIdx(e.prev());\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".clear();\n"
		      << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);\n";
	});
	cpp() << "\n"
	      << "\tgetGraph().getEventLabel(e)->set" << ((status == VarStatus::View) ? "Views" : "Calculated") << "({";
	std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &a){
		cpp() << "{}, ";
	});
	cpp() << "});\n";
	std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &a){
		cpp() << "\tvisitCalc" << GET_ID(id, ids[a]) << "(e, calcRes);\n";
	});
	cpp() << "\treturn calcRes;\n"
	      << "}\n";
}

void Printer::printPPoRfHpp(const NFA &nfa, bool deps)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitPPoRfXX for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tvoid visitPPoRf" << ids[&*s] << "(const Event &e, "
		      			     << (deps ? "DepView" : "View") << " &pporf);\n";
	});
	hpp() << "\n";

	/* calcPPoRfBefore for the automaton */
	hpp() << "\t" << (deps ? "DepView" : "View") << " calcPPoRfBefore(const Event &e);\n";
	hpp() << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeStatus> visitedPPoRf" << ids[&*s] << ";\n";
	});
	hpp() << "\n";
}

void Printer::printPPoRfCpp(const NFA &nfa, bool deps)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "void " << className << "::visitPPoRf" << ids[&*s] << "(const Event &e, "
		      					        << (deps ? "DepView" : "View") << " &pporf)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(e);\n"
		      << "\tauto t = 0u;\n"
		      << "\n"
		      << "\tvisitedPPoRf" << ids[&*s] << "[lab->getStamp()] = NodeStatus::entered;\n";
		if (nfa.isStarting(&*s))
			cpp() << "\tpporf.updateIdx(e);\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto status = visitedPPoRf" << ids[t.dest] << "[g.getEventLabel(p)->getStamp()];\n"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitPPoRf" << ids[t.dest] << "(p, pporf);\n"
			      <<"\t}\n";
		});
		cpp() << "\tvisitedPPoRf" << ids[&*s] << "[lab->getStamp()] = NodeStatus::left;\n"
		      << "}\n"
		      << "\n";
	});

	cpp() << (deps ? "DepView " : "View ") << className << "::calcPPoRfBefore(const Event &e)\n"
	      << "{\n"
	      << "\t" << (deps ? "DepView" : "View") << " pporf;\n"
	      << "\tpporf.updateIdx(e);\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\tvisitedPPoRf" << ids[&*s] << ".clear();\n"
		      << "\tvisitedPPoRf" << ids[&*s] << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);\n";
	});
	cpp() << "\n";
	std::for_each(nfa.accept_begin(), nfa.accept_end(), [&](auto &a){
		cpp() << "\tvisitPPoRf" << ids[a] << "(e, pporf);\n";
	});
	cpp() << "\treturn pporf;\n"
	      << "}\n";
}

void Printer::printInclusionHpp(const NFA &lhs, const NFA &rhs, unsigned id)
{
	auto ids = assignStateIDs(rhs.states_begin(), rhs.states_end());

	/* visitInclusionXX for each state */
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		hpp() << "\tvoid visitInclusion" << GET_ID(id, ids[&*s]) << "(const Event &e)" << ";\n";
	});
	hpp() << "\n";

	/* checkInclusionX for the automaton */
	hpp() << "\tbool checkInclusion" << id << "(const Event &e)" << ";\n"
	      << "\n";

	/* status arrays */
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		hpp() << "\tstatic inline thread_local std::vector<NodeStatus> visitedInclusion" << GET_ID(id, ids[&*s]) << ";\n";
	});
	hpp() << "\n";
}

void Printer::printInclusionCpp(const NFA &lhs, const NFA &rhs, unsigned id)
{
	auto ids = assignStateIDs(rhs.states_begin(), rhs.states_end());

	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		cpp() << "void " << className << "::visitInclusion" << GET_ID(id, ids[&*s]) << "(const Event &e)\n"
		      << "{\n"
		      << "\tauto &g = getGraph();\n"
		      << "\tauto *lab = g.getEventLabel(" << "e" << ");\n"
		      << "\n"
		      << "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::entered;\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			cpp () << "\t";
			printTransLabel(&t.label, "p", "lab");
			cpp() << " {\n"
			      << "\t\tauto status = visitedInclusion" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()]\n;"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitInclusion" << GET_ID(id, ids[t.dest]) << "(p);\n"
			      << "\t}\n";
		});
		if (rhs.isStarting(&*s))
			cpp() << "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::left;\n"
			      << "}\n"
			      << "\n";
	});

	cpp() << "bool " << className << "::checkInclusion(const Event &e)\n"
	      << "{\n";
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		cpp() << "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << ".clear();\n"
		      << "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);\n";
	});
	std::for_each(rhs.start_begin(), rhs.start_end(), [&](auto &s){
		cpp() << "\t\tvisitInclusion" << GET_ID(id, ids[s]) << "(e);\n";
	});

	/* TODO */
	cpp() << "\treturn true;\n"
	      << "}\n"
	      << "\n";
}
