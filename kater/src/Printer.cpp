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

void Printer::printPredLabel(std::ostream &ostr, const PredLabel *p, const std::string &res, const std::string &arg, const std::string &ident)
{
	ostr << ident << "if (auto " << res << " = " << arg << "->getPos()";

	auto first = true;
	if (!p->hasPreds()) {
		ostr << ") {\n";
		return;
	}

	ostr << ";";
	for (auto it = p->pred_begin(), ie = p->pred_end(); it != ie; ++it) {
		if (first) {
			ostr << " ";
			first = false;
		} else {
			ostr << " && ";
		}

		auto s = builtinPredicates[*it].genmcString;
		s.replace(s.find_first_of('#'), 1, arg);
		ostr << s;
	}
	ostr << ") {\n";
}

void Printer::printRelLabel(std::ostream& ostr, const RelLabel *r, const std::string &res, const std::string &arg, const std::string &ident)
{
	if (r->isBuiltin()) {
		const auto &n = builtinRelations[r->getTrans()];
		const auto &s = r->isFlipped() ? n.predString : n.succString;
		// if ((n.type == RelType::OneOne) || (flipped && n.type == RelType::ManyOne))
		// 	ostr << "\tif (auto " << res << " = " << s << ") {\n";
		// else
		ostr << ident << "for (auto &" << res << " : " << s << "(g, " << arg << "->getPos())) {\n";
		return;
	}
	auto isView = viewCalcs.count(r->getCalcIndex());
	auto form = isView ? "view" : "calculated";
	ostr << (isView ? ident + "t = 0u;\n" : "");
	ostr << ident << "for (auto &" << "i" << " : " << arg << "->" << form << "(" << getCalcIdx(r->getCalcIndex()) << ")) {\n"
	     << ident << "\tauto p = " << (isView ? "Event(t++, i)" : "i") << ";\n";
	return;
}

void Printer::printTransLabel(const TransLabel *t, const std::string &res, const std::string &arg, const std::string &ident)
{
	if (auto *p = dynamic_cast<const PredLabel *>(t))
		printPredLabel(cpp(), p, res, arg, ident);
	else if (auto *r = dynamic_cast<const RelLabel *>(t))
		printRelLabel(cpp(), r, res, arg, ident);
	else
		assert(0);
}

void Printer::printHppHeader()
{
	hpp() << genmcCopyright << "\n"
	      << katerNotice << "\n";

	hpp() << "#ifndef " << guardName << "\n"
	      << "#define " << guardName << "\n"
	      << "\n"
	      << "#include \"ExecutionGraph.hpp\"\n"
	      << "#include \"GraphIterators.hpp\"\n"
	      << "#include \"PersistencyChecker.hpp\"\n"
	      << "#include \"VSet.hpp\"\n"
	      << "#include <vector>\n"
	      << "\n"
	      << "class " << className << " {\n"
	      << "\n"
	      << "private:\n"
	      << "\tenum class NodeStatus { unseen, entered, left };\n"
	      << "\n"
	      << "\tstruct NodeCountStatus {\n"
	      << "\t\tNodeCountStatus() = default;\n"
	      << "\t\tNodeCountStatus(unsigned c, NodeStatus s) : count(c), status(s) {}\n"
	      << "\t\tunsigned count = 0;\n"
	      << "\t\tNodeStatus status = NodeStatus::unseen;\n"
	      << "\t};\n"
	      << "\n"
	      << "public:\n"
	      << "\t" << className << "(ExecutionGraph &g) : g(g) {}\n"
	      << "\n"
	      << "\tstd::vector<VSet<Event>> calculateSaved(const Event &e);\n"
	      << "\tstd::vector<View> calculateViews(const Event &e);\n"
	      << "\tbool isConsistent(const Event &e);\n"
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
		hpp() << "\tstd::vector<NodeCountStatus> visitedAcyclic" << ids[&*s] << ";\n";
	});
	hpp() << "\n";

	/* accepting counter */
	hpp() << "\tunsigned visitedAccepting = 0;\n";
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

		if (s->isStarting())
			cpp() << "\t++visitedAccepting;\n";
		cpp() << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp()] = "
								"{ visitedAccepting, NodeStatus::entered };\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			printTransLabel(&*t.label, "p", "lab", "\t");
			cpp() << "\t\tauto &node = visitedAcyclic" << ids[t.dest] << "[g.getEventLabel(p)->getStamp()];\n"
			      << "\t\tif (node.status == NodeStatus::unseen && !visitAcyclic" << ids[t.dest] << "(p))\n"
			      << "\t\t\treturn false;\n"
			      << "\t\telse if (node.status == NodeStatus::entered && visitedAccepting > node.count)\n"
			      << "\t\t\treturn false;\n"
			      <<"\t}\n";
		});
		if (s->isStarting())
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
	cpp() << "\treturn true\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		cpp() << "\t\t&& visitAcyclic" << ids[&*s] << "(e)"
		      << (&*s == (--nfa.states_end())->get() ? ";\n" : "\n");
	});
	cpp() << "}\n"
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
		hpp() << "\tstd::vector<NodeStatus> visitedCalc" << GET_ID(id, ids[&*s]) << ";\n";
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
		if (s->isStarting()) {
			if (status != VarStatus::View)
				cpp() << "\tcalcRes.insert(e);\n";
			if (status == VarStatus::Reduce) {
				cpp() << "\tfor (const auto &p : lab->calculated(" << getCalcIdx(id) << ")) {\n"
				      << "\t\tcalcRes.erase(p);\n";
				std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
					if (a->isAccepting())
						cpp() << "\t\tvisitedCalc" << GET_ID(id, ids[&*a]) << "[g.getEventLabel(p)->getStamp()] = NodeStatus::left;\n";
				});
				cpp() << "\t}\n";
			} else if (status == VarStatus::View) {
				cpp() << "\tcalcRes.update(lab->view(" << getCalcIdx(id) << "));\n"
				      << "\tcalcRes.updateIdx(lab->getPos());\n";
			}
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			printTransLabel(&*t.label, "p", "lab", "\t");
			cpp() << "\t\tauto status = visitedCalc" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()];\n"
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
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
		if (a->isAccepting())
			cpp() << "{}, ";
	});
	cpp() << "});\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
		if (a->isAccepting())
			cpp() << "\tvisitCalc" << GET_ID(id, ids[&*a]) << "(e, calcRes);\n";
	});
	cpp() << "\treturn calcRes;\n"
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
		hpp() << "\tstd::vector<NodeStatus> visitedInclusion" << GET_ID(id, ids[&*s]) << ";\n";
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
			printTransLabel(&*t.label, "p", "lab", "\t");
			cpp() << "\t\tauto status = visitedInclusion" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()]\n;"
			      << "\t\tif (status == NodeStatus::unseen)\n"
			      << "\t\t\tvisitInclusion" << GET_ID(id, ids[t.dest]) << "(p);\n"
			      << "\t}\n";
		});
		if (s->isStarting())
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
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		if (s->isStarting())
			cpp() << "\t\tvisitInclusion" << GET_ID(id, ids[&*s]) << "(e);\n";
	});

	/* TODO */
	cpp() << "\treturn true;\n"
	      << "}\n"
	      << "\n";
}
