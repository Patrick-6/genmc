#include "Config.hpp"
#include "Error.hpp"
#include "Printer.hpp"

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

void Printer::printPredLabel(std::ostream &ostr, const PredLabel *p, std::string res, std::string arg)
{
	ostr << "if (auto " << res << " = " << arg << "->getPos()";

	auto first = true;
	if (!p->hasPreds()) {
		ostr << ")";
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
	ostr << ")";
}

void Printer::printRelLabel(std::ostream& ostr, const RelLabel *r, std::string res, std::string arg)
{
	if (r->isBuiltin()) {
		const auto &n = builtinRelations[r->getTrans()];
		const auto &s = r->isFlipped() ? n.predString : n.succString;
		// if ((n.type == RelType::OneOne) || (flipped && n.type == RelType::ManyOne))
		// 	ostr << "\tif (auto " << res << " = " << s << ") {\n";
		// else
		ostr << "for (auto &" << res << " : " << s << "(g, " << arg << "->getPos()))";
		return;
	}
	ostr << "for (auto &" << res << " : " << arg << "->calculated(" << r->getCalcIndex() << "))";
	return;
}

void Printer::printTransLabel(const TransLabel *t, std::string res, std::string arg)
{
	if (auto *p = dynamic_cast<const PredLabel *>(t))
		printPredLabel(*outCpp, p, res, arg);
	else if (auto *r = dynamic_cast<const RelLabel *>(t))
		printRelLabel(*outCpp, r, res, arg);
	else
		assert(0);
}

void Printer::printHppHeader()
{
	*outHpp << genmcCopyright << "\n"
		<< katerNotice << "\n";

	*outHpp	<< "#ifndef " << guardName << "\n"
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
		<< "\tstd::vector<VSet<Event>> calculateAll(const Event &e);\n"
		<< "\tbool isConsistent(const Event &e);\n"
		<< "\n"
		<< "private:\n";
}

void Printer::printCppHeader()
{
	*outCpp << genmcCopyright << "\n"
		<< katerNotice << "\n";

	*outCpp <<  "#include \"" << className << ".hpp\"\n"
		<< "\n";
}

void Printer::printHppFooter()
{
	*outHpp << "\tstd::vector<VSet<Event>> calculated;\n"
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
		printCalculatorHpp(nfaStatus.first, i++);
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

	/* Print calculators + calculateAll() */
	auto i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfaStatus){
		printCalculatorCpp(nfaStatus.first, i++, nfaStatus.second);
	});
	*outCpp << "std::vector<VSet<Event>> " << className << "::calculateAll(const Event &e)\n"
		<< "{\n";
	i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfa){
		*outCpp << "\tcalculated.push_back(calculate" << i++ << "(e));\n";
	});
	*outCpp << "\treturn std::move(calculated);\n"
		<< "}\n"
		<< "\n";

	i = 0u;
	std::for_each(cnfas.incl_begin(), cnfas.incl_end(), [&](auto &nfaPair){
		printInclusionCpp(nfaPair.lhs, nfaPair.rhs, i++);
	});

	/* Print acyclicity routines + isConsistent() */
	printAcyclicCpp(cnfas.getAcyclic());
	*outCpp << "bool " << className << "::isConsistent(const Event &e)\n"
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

	/* visitAcyclicX for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outHpp << "\tbool visitAcyclic" << ids[&*s] << "(const Event &e)" << ";\n";
	});
	*outHpp << "\n";

	/* isAcyclic for the automaton */
	*outHpp << "\tbool isAcyclic(const Event &e)" << ";\n"
		<< "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outHpp << "\tstd::vector<NodeCountStatus> visitedAcyclic" << ids[&*s] << ";\n";
	});
	*outHpp << "\n";

	/* accepting counter */
	*outHpp << "\tunsigned visitedAccepting = 0;\n";
}

void Printer::printAcyclicCpp(const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* Print a "visitAcyclicXX" for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outCpp << "bool " << className << "::visitAcyclic" << ids[&*s] << "(const Event &e)\n"
			<< "{\n"
			<< "\tauto &g = getGraph();\n"
			<< "\tauto *lab = g.getEventLabel(" << "e" << ");\n"
			<< "\n";

		if (s->isStarting())
			*outCpp << "\t++visitedAccepting;\n";
		*outCpp << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp()] = "
								"{ visitedAccepting, NodeStatus::entered };\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			*outCpp << "\t";
			printTransLabel(&*t.label, "p", "lab");
			*outCpp << " {\n"
				<< "\t\tauto &node = visitedAcyclic" << ids[t.dest] << "[g.getEventLabel(p)->getStamp()];\n"
				<< "\t\tif (node.status == NodeStatus::unseen && !visitAcyclic" << ids[t.dest] << "(p))\n"
				<< "\t\t\treturn false;\n"
				<< "\t\telse if (node.status == NodeStatus::entered && visitedAccepting > node.count)\n"
				<< "\t\t\treturn false;\n"
				<<"\t}\n";
		});
		if (s->isStarting())
			*outCpp << "\t--visitedAccepting;\n";
		*outCpp << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp()] = "
								"{ visitedAccepting, NodeStatus::left };\n"
			<< "\treturn true;\n"
			<< "}\n"
			<< "\n";
	});

	/* Print a "isAcyclicX" for the automaton */
	*outCpp << "bool " << className << "::isAcyclic(const Event &e)\n"
		<< "{\n"
		<< "\tvisitedAccepting = 0;\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outCpp << "\tvisitedAcyclic" << ids[&*s] << ".clear();\n"
			<< "\tvisitedAcyclic" << ids[&*s] << ".resize(g.getMaxStamp() + 1);\n";
	});
	*outCpp << "\treturn true\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outCpp << "\t\t&& visitAcyclic" << ids[&*s] << "(e)"
			<< (&*s == (--nfa.states_end())->get() ? ";\n" : "\n");
	});
	*outCpp << "}\n"
		<< "\n";
}

void Printer::printCalculatorHpp(const NFA &nfa, unsigned id)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* visitCalcXX for each state */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outHpp << "\tvoid visitCalc" << GET_ID(id, ids[&*s]) << "(const Event &e, VSet<Event> &calcRes);\n";
	});
	*outHpp << "\n";

	/* calculateX for the automaton */
	*outHpp << "\tVSet<Event> calculate" << id << "(const Event &e);\n";
	*outHpp << "\n";

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outHpp << "\tstd::vector<NodeStatus> visitedCalc" << GET_ID(id, ids[&*s]) << ";\n";
	});
	*outHpp << "\n";
}

void Printer::printCalculatorCpp(const NFA &nfa, unsigned id, VarStatus reduce)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outCpp << "void " << className << "::visitCalc" << GET_ID(id, ids[&*s]) << "(const Event &e, VSet<Event> &calcRes)\n"
			<< "{\n"
			<< "\tauto &g = getGraph();\n"
			<< "\tauto *lab = g.getEventLabel(e);\n"
			<< "\n"
			<< "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::entered;\n";
		if (s->isStarting()) {
			*outCpp << "\tcalcRes.insert(e);\n";
			if (reduce == VarStatus::Reduce) {
				*outCpp << "\tfor (const auto &p : lab->calculated(" << id << ")) {\n"
					<< "\t\tcalcRes.erase(p);\n";
				std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
					if (a->isAccepting())
						*outCpp << "\t\tvisitedCalc" << GET_ID(id, ids[&*a]) << "[g.getEventLabel(p)->getStamp()] = NodeStatus::left;\n";
				});
				*outCpp << "\t}\n";
			}
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			*outCpp << "\t";
			printTransLabel(&*t.label, "p", "lab");
			*outCpp << " {\n"
				<< "\t\tauto status = visitedCalc" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()];\n"
				<< "\t\tif (status == NodeStatus::unseen)\n"
				<< "\t\t\tvisitCalc" << GET_ID(id, ids[t.dest]) << "(p, calcRes);\n"
				<<"\t}\n";
		});
		*outCpp << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::left;\n"
			<< "}\n"
			<< "\n";
	});

	*outCpp << "VSet<Event> " << className << "::calculate" << id << "(const Event &e)\n"
		<< "{\n"
		<< "\tVSet<Event> calcRes;\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outCpp << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".clear();\n"
			<< "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);\n";
	});
	*outCpp << "\n"
		<< "\tgetGraph().getEventLabel(e)->setCalculated({";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
		if (a->isAccepting())
			*outCpp << "{}, ";
	});
	*outCpp << "});\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
		if (a->isAccepting())
			*outCpp << "\tvisitCalc" << GET_ID(id, ids[&*a]) << "(e, calcRes);\n";
	});
	*outCpp << "\treturn calcRes;\n"
		<< "}\n";
}

void Printer::printInclusionHpp(const NFA &lhs, const NFA &rhs, unsigned id)
{
	auto ids = assignStateIDs(rhs.states_begin(), rhs.states_end());

	/* visitInclusionXX for each state */
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		*outHpp << "\tvoid visitInclusion" << GET_ID(id, ids[&*s]) << "(const Event &e)" << ";\n";
	});
	*outHpp << "\n";

	/* checkInclusionX for the automaton */
	*outHpp << "\tbool checkInclusion" << id << "(const Event &e)" << ";\n"
		<< "\n";

	/* status arrays */
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		*outHpp << "\tstd::vector<NodeStatus> visitedInclusion" << GET_ID(id, ids[&*s]) << ";\n";
	});
	*outHpp << "\n";
}

void Printer::printInclusionCpp(const NFA &lhs, const NFA &rhs, unsigned id)
{
	auto ids = assignStateIDs(rhs.states_begin(), rhs.states_end());

	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		*outCpp << "void " << className << "::visitInclusion" << GET_ID(id, ids[&*s]) << "(const Event &e)\n"
			<< "{\n"
			<< "\tauto &g = getGraph();\n"
			<< "\tauto *lab = g.getEventLabel(" << "e" << ");\n"
			<< "\n"
			<< "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::entered;\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			*outCpp << "\t";
			printTransLabel(&*t.label, "p", "lab");
			*outCpp << " {\n"
				<< "\t\tauto status = visitedInclusion" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()]\n;"
				<< "\t\tif (status == NodeStatus::unseen)\n"
				<< "\t\t\tvisitInclusion" << GET_ID(id, ids[t.dest]) << "(p);\n"
				<< "\t}\n";
		});
		if (s->isStarting())
			*outCpp << "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::left;\n"
				<< "}\n"
				<< "\n";
	});

	*outCpp << "bool " << className << "::checkInclusion(const Event &e)\n"
		<< "{\n";
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		*outCpp << "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << ".clear();\n"
			<< "\tvisitedInclusion" << GET_ID(id, ids[&*s]) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);\n";
	});
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		if (s->isStarting())
			*outCpp << "\t\tvisitInclusion" << GET_ID(id, ids[&*s]) << "(e);\n";
	});

	/* TODO */
	*outCpp << "\treturn true;\n"
		<< "}\n"
		<< "\n";
}
