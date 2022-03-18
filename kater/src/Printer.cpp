#include "Config.hpp"
#include "Error.hpp"
#include "Printer.hpp"

std::string katerNotice = "/* This file is generated automatically by Kater -- do not edit. */";

void openFileForWriting(const std::string &filename, std::ofstream &fout)
{
	fout.open(filename, std::ios_base::out);
	if (!fout.is_open()) {
		std::cerr << "Could not open file for writing: " << filename << "\n";
		exit(EPRINT);
	}
}

Printer::Printer(const std::string &outPrefix)
{
	auto name = outPrefix != "" ? outPrefix : "Demo";

	/* Construct all the names to be used */
	std::transform(name.begin(), name.end(), std::back_inserter(prefix), ::toupper);
	className = prefix + "Checker";
	guardName = std::string("__") + prefix + "_CHECKER_HPP__";

	/* Open required streams */
	if (outPrefix != "") {
		openFileForWriting(className + ".hpp", foutHpp);
		outHpp = &foutHpp;
		openFileForWriting(className + ".cpp", foutCpp);
		outCpp = &foutCpp;
	}
}

void Printer::printHppHeader()
{
	*outHpp << katerNotice << "\n"
		<< "\n";

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
		<< "public:\n"
		<< "\tvoid computeCalcs(const Event &e);\n"
		<< "\tvoid isConsistent(const Event &e);\n"
		<< "\n"
		<< "private:\n";
}

void Printer::printCppHeader()
{
	*outCpp << katerNotice << "\n"
		<< "\n";

	*outCpp <<  "#include \"" << className << ".hpp\"\n"
		<< "\n";
}

void Printer::printHppFooter()
{
	*outHpp << "\tExecutionGraph &g;\n"
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
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfa){
		printCalculatorHpp(nfa, i++);
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

	auto i = 0u;
	std::for_each(cnfas.save_begin(), cnfas.save_end(), [&](auto &nfa){
		printCalculatorCpp(nfa, i++, VarStatus::Normal);
	});

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
		*outHpp << "\tstd::vector<NodeStatus> visitedAcyclic" << ids[&*s] << ";\n";
	});
	*outHpp << "\n";

	/* accepting counter */
	*outHpp << "\tint visitedAccepting = 0;\n";
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
			<< "\n"
			<< "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp()] = NodeStatus::entered;\n";
		if (s->isStarting())
			*outCpp << "\t++visitedAccepting;\n";
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(*outCpp, "e", "p");
			*outCpp << "\t\tauto status = visitedAcyclic" << ids[t.dest] << "[g.getEventLabel(p)->getStamp()];\n"
				<< "\t\tif (status == NodeStatus::unseen && !visitAcyclic" << ids[t.dest] << "(p))\n"
				<< "\t\t\treturn false;\n"
				<< "\t\telse if (status == NodeStatus::entered && visitedAccepting)\n"
				<< "\t\t\treturn false;\n"
				<<"\t}\n";
		});
		if (s->isStarting())
			*outCpp << "\t--visitedAccepting;";
		*outCpp << "\tvisitedAcyclic" << ids[&*s] << "[lab->getStamp()] = NodeStatus::left;\n"
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
			<< "\tvisitedAcyclic" << ids[&*s] << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);\n";
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
		*outHpp << "\tvoid visitCalc" << GET_ID(id, ids[&*s]) << "(constEvent &e);\n";
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
		*outCpp << "void " << className << "::visitCalc" << GET_ID(id, ids[&*s]) << "(const Event &e)\n"
			<< "{\n"
			<< "\tauto &g = getGraph();\n"
			<< "\tauto *lab = g.getEventLabel(e);\n"
			<< "\n"
			<< "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::entered;\n";
		if (s->isStarting()) {
			*outCpp << "\tcalcRes.insert(e);\n";
			//
			// FIXME
			//
			// if (reduce == VarStatus::Reduce) {
			// 	PRINT_LINE("\tfor (const auto &p : calc" << whichCalc << "_preds(g, e)) {");
			// 	PRINT_LINE("\t\tcalcRes.erase(p);");
			// 	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
			// 		if (a->isAccepting())
			// 			PRINT_LINE("\t\t" << VISITED_IDX(&*a, "p") << " = true;");
			// 	});
			// 	PRINT_LINE("\t}");
			// }
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(*outCpp, "e", "p");
			*outCpp << "\t\tauto status = visitedCalc" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()];\n"
				<< "\t\tif (status == NodeStatus::unseen)\n"
				<< "\t\t\tvisitCalc" << GET_ID(id, ids[t.dest]) << "(p)\n"
				<<"\t}\n";
		});
		*outCpp << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << "[lab->getStamp()] = NodeStatus::left;\n"
			<< "}\n"
			<< "\n";
	});

	*outCpp << "VSet<Event> " << className << "::calculate(const Event &e)\n"
		<< "{\n"
		<< "\tVSet<Event> calcRes;\n";
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		*outCpp << "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".clear();\n"
			<< "\tvisitedCalc" << GET_ID(id, ids[&*s]) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);\n";
	});
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
		if (a->isAccepting())
			*outCpp << "\tvisitCalc" << GET_ID(id, ids[&*a]) << "(e);\n";
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
			t.label->output_for_genmc(*outCpp, "e", "p");
			*outCpp << "\t\tauto status = visitedInclusion" << GET_ID(id, ids[t.dest]) << "[g.getEventLabel(p)->getStamp()]\n;"
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
