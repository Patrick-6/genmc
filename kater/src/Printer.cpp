#include "Config.hpp"
#include "Printer.hpp"
#include <iostream>
#include <fstream>

template<typename ITER>
std::unordered_map<NFA::State *, unsigned> assignStateIDs(ITER &&begin, ITER &&end)
{
	std::unordered_map<NFA::State *, unsigned> result;

	auto i = 0u;
	std::for_each(begin, end, [&](auto &s){ result[&*s] = i++; });
	return result;
}

Printer::Printer(const NFAs *arg) {
	auto name = config.outPrefix != "" ? config.outPrefix :
		config.inputFile.substr(0, config.inputFile.find_last_of("."));
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);
	className = name + "Checker";
	guardName = std::string("__") + name + "_CHECKER_HPP__";
	res = arg;
}

void Printer::output()
{
	outputHeader();
	outputImpl();
}


static void printKaterNotice(std::ostream &out)
{
	out << "/* This file is generated automatically by Kater -- do not edit. */\n\n";
	return;
}

#define PRINT_LINE(line) (*out) << line << "\n"

void Printer::outputHeader()
{
	auto name = config.outPrefix != "" ? config.outPrefix :
		config.inputFile.substr(0, config.inputFile.find_last_of("."));
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);

	auto className = name;
	className += "Checker";

	std::string guardName = "__";
	guardName += name;
	guardName += "_CHECKER_HPP__";

	std::ostream* out = &std::cout;
	std::ofstream fout;
	if (config.outPrefix != "") {
		fout.open(className + ".hpp");
		if (!fout.is_open())
			return;
		out = &fout;
	}

	printKaterNotice(*out);

	PRINT_LINE("#ifndef " << guardName);
	PRINT_LINE("#define " << guardName);

	PRINT_LINE("");
	PRINT_LINE("#include \"ExecutionGraph.hpp\"");
	PRINT_LINE("#include \"GraphIterators.hpp\"");
	PRINT_LINE("#include \"PersistencyChecker.hpp\"");
	PRINT_LINE("#include \"VSet.hpp\"");
	PRINT_LINE("#include <vector>");

	PRINT_LINE("");
	PRINT_LINE("class " << className << " {");

	PRINT_LINE("");
	PRINT_LINE("private:");
	PRINT_LINE("\tenum class NodeStatus { unseen, entered, left };");

	PRINT_LINE("public:");

	PRINT_LINE("\tvoid computeCalcs(const Event &e);");
	PRINT_LINE("\tvoid isConsistent(const Event &e);");
	PRINT_LINE("");

	PRINT_LINE("");
	PRINT_LINE("private:");

	for (int i = 0; i < res->nsaved.size(); ++i)
		printCalculatorHeader(*out, res->nsaved[i].first, i);
	for (int i = 0; i < res->nincl.size(); ++i)
		printInclusionHeader(*out, res->nincl[i].first, res->nincl[i].second, i);
	printAcyclicHeader(*out, res->nfa_acyc);

	PRINT_LINE("\tExecutionGraph &g;");

	PRINT_LINE("");
	PRINT_LINE("\tExecutionGraph &getGraph() { return g; }");

	PRINT_LINE("};");

	PRINT_LINE("");
	PRINT_LINE("#endif /* " << guardName << " */");
}

void Printer::outputImpl()
{
	auto name = config.outPrefix != "" ? config.outPrefix :
		config.inputFile.substr(0, config.inputFile.find_last_of("."));
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);

	auto className = name;
	className += "Checker";

	std::ostream* out = &std::cout;
	std::ofstream fout;
	if (config.outPrefix != "") {
		fout.open(className + ".cpp");
		if (!fout.is_open())
			return;
		out = &fout;
	}

	printKaterNotice(*out);

	PRINT_LINE("#include \"" << className << ".hpp\"");
	PRINT_LINE("");

	for (int i = 0; i < res->nsaved.size(); ++i)
		printCalculatorImpl(*out, res->nsaved[i].first, i, res->nsaved[i].second);
	for (int i = 0; i < res->nincl.size(); ++i)
		printInclusionImpl(*out, res->nincl[i].first, res->nincl[i].second, i);
	printAcyclicImpl(*out, res->nfa_acyc);
}

#undef PRINT_LINE
#define PRINT_LINE(line) fout << line << "\n"

/* --------------------------------------------------------------------- */
/*                          Calculators                                  */
/* --------------------------------------------------------------------- */

#define VSET             "VSet<Event>"
#define CALC             "calculate" << whichCalc << "(const ExecutionGraph &g, const Event &e)"
#define VISIT_PROC(i)    "visit" << whichCalc << "_" << i
#define VISIT_CALL(i,e)  VISIT_PROC(i) << "(calcRes, " << e << ");"
#define VISIT_PARAMS	 "(" << VSET << " &calcRes, const ExecutionGraph &g, const Event &e)"
#define VISITED_ARR	 "visitedCalc" << whichCalc
#define VISITED_IDX(i,e) VISITED_ARR << "[g.getEventLabel(" << e \
			 << ")->getStamp()][" << i << "]"

void Printer::printCalculatorHeader(std::ostream &fout, const NFA &nfa, int whichCalc)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	PRINT_LINE("\t" << VSET << " " << CALC << ";");

	/* visit procedures */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tvoid " << VISIT_PROC(ids[&*s]) << VISIT_PARAMS);
	});
	PRINT_LINE("");

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tstd::vector<NodeStatus> " << VISITED_ARR << ids[&*s] << ";");
	});
	PRINT_LINE("");

	/* calculated relation */
	PRINT_LINE("\tVSet<Event> calculated" << whichCalc << ";");
	PRINT_LINE("");
}

void Printer::printCalculatorImpl(std::ostream &fout, const NFA &nfa, int whichCalc, VarStatus reduce)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("void " << className << "::" << VISIT_PROC(ids[&*s]) << VISIT_PARAMS);
		PRINT_LINE("{");
		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("\tauto *lab = g.getEventLabel(" << "e" << ");");

		PRINT_LINE("");
		PRINT_LINE("\t" << VISITED_ARR << ids[&*s] << "[lab->getStamp()]" << " = NodeStatus::entered;");
		if (s->isStarting()) {
			PRINT_LINE("\tcalcRes.insert(e);");
			if (reduce == VarStatus::Reduce) {
				PRINT_LINE("\tfor (const auto &p : calc" << whichCalc << "_preds(g, e)) {");
				PRINT_LINE("\t\tcalcRes.erase(p);");
				std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
					if (a->isAccepting())
						PRINT_LINE("\t\t" << VISITED_IDX(ids[&*a], "p") << " = true;");
				});
				PRINT_LINE("\t}");
			}
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(fout, "e", "p");
			PRINT_LINE("\t\tauto status = " << VISITED_IDX(ids[t.dest], "p") << ";");
			PRINT_LINE("\t\tif (status == NodeStatus::unseen)");
			PRINT_LINE("\t\t\t" << VISIT_CALL(ids[t.dest], "p"));
			PRINT_LINE("\t}");
		});
		PRINT_LINE("\t" << VISITED_ARR << ids[&*s] << "[lab->getStamp()]" << " = NodeStatus::left;");
		PRINT_LINE("}");
		PRINT_LINE("");
	});

	PRINT_LINE(VSET << " " << className << "::" << CALC);
	PRINT_LINE("{");
	PRINT_LINE("\t" << VSET << " calcRes;");
	PRINT_LINE("\t" << VISITED_ARR << ".clear();");
	PRINT_LINE("\t" << VISITED_ARR << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
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

/* --------------------------------------------------------------------- */
/*                      Acyclicity checker                               */
/* --------------------------------------------------------------------- */

#define VISIT_PROC(i)      "visitAcyclic" << i
#define VISIT_CALL(i,e)    VISIT_PROC(i) << "(" << e << ")"
#define VISIT_PARAMS	   "(const Event &e)"
#define VISITED_ARR	   "visitedAcyclic"
#define VISITED_IDX(i,e)   VISITED_ARR << i << "[g.getEventLabel(" << e << ")->getStamp()]"
#define VISITED_ACCEPTING  "visitedAccepting"
#define TOPLEVEL           "isAcyclic" << VISIT_PARAMS

void Printer::printAcyclicHeader(std::ostream &fout, const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* entry point */
	PRINT_LINE("");
	PRINT_LINE("\tbool " << TOPLEVEL << ";");

	/* visit procedures */
	PRINT_LINE("");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tbool " << VISIT_PROC(ids[&*s]) << VISIT_PARAMS << ";");
	});

	/* status arrays */
	PRINT_LINE("");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tstd::vector<NodeStatus> " << VISITED_ARR << ids[&*s] << ";");
	});

	/* accepting counter */
	PRINT_LINE("");
	PRINT_LINE("\tint " <<  VISITED_ACCEPTING << " = 0;");
}

void Printer::printAcyclicImpl(std::ostream &fout, const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("bool " << className << "::" << VISIT_PROC(ids[&*s]) << VISIT_PARAMS);
		PRINT_LINE("{");

		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("\tauto *lab = g.getEventLabel(" << "e" << ");");

		PRINT_LINE("");
		PRINT_LINE("\t" << VISITED_ARR << ids[&*s] << "[lab->getStamp()]" << " = NodeStatus::entered;");
		if (s->isStarting())
			PRINT_LINE("\t++" << VISITED_ACCEPTING << ";");
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(fout, "e", "p");
			PRINT_LINE("\t\tauto status = " << VISITED_IDX(ids[t.dest], "p") << ";");
			PRINT_LINE("\t\tif (status == NodeStatus::unseen && !" << VISIT_CALL(ids[t.dest], "p") << ")");
			PRINT_LINE("\t\t\treturn false;");
			PRINT_LINE("\t\telse if (status == NodeStatus::entered && visitedAccepting)");
			PRINT_LINE("\t\t\treturn false;");
			PRINT_LINE("\t}");
		});
		if (s->isStarting())
			PRINT_LINE("\t--" << VISITED_ACCEPTING << ";");
		PRINT_LINE("\t" << VISITED_ARR << ids[&*s] << "[lab->getStamp()]" << " = NodeStatus::left;");
		PRINT_LINE("\treturn true;");
		PRINT_LINE("}");
		PRINT_LINE("");
	});

	PRINT_LINE("bool " << className << "::" << TOPLEVEL);
	PRINT_LINE("{");
	PRINT_LINE("\t" << VISITED_ACCEPTING << " = 0;");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\t" << VISITED_ARR << ids[&*s] << ".clear();");
		PRINT_LINE("\t" << VISITED_ARR << ids[&*s] << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);");
	});
	PRINT_LINE("\treturn true");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE ("\t\t&& " << VISIT_CALL(ids[&*s], "e")
			    << (&*s == (--nfa.states_end())->get() ? ";" : ""));
	});
	PRINT_LINE("}");
	PRINT_LINE("");
}


#undef VISIT_PROC
#undef VISIT_CALL
#undef VISIT_PARAMS
#undef VISITED_ARR
#undef VISITED_IDX
#undef VISITED_ACCEPTING
#undef TOPLEVEL

/* --------------------------------------------------------------------- */
/*                      Inclusion checkers                               */
/* --------------------------------------------------------------------- */

#define VISIT_PROC(i)      "visitInclusion" << whichInclusion << "_" << ids[i]
#define VISIT_CALL(i,e)    VISIT_PROC(i) << "(" << e << ")"
#define VISIT_PARAMS	   "(const Event &e)"
#define VISITED_ARR(i)	   "visitedInclusion" << whichInclusion << "_" << ids[i]
#define VISITED_IDX(i,e)   VISITED_ARR(i) << "[g.getEventLabel(" << e << ")->getStamp()]"
#define TOPLEVEL           "checkInclusion" << whichInclusion << VISIT_PARAMS

void Printer::printInclusionHeader(std::ostream &fout, const NFA &lhs, const NFA &rhs,
				   int whichInclusion)
{
	auto ids = assignStateIDs(rhs.states_begin(), rhs.states_end());

	/* entry point */
	PRINT_LINE("");
	PRINT_LINE("\tbool " << TOPLEVEL << ";");

	/* visit procedures */
	PRINT_LINE("");
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		PRINT_LINE("\tvoid " << VISIT_PROC(&*s) << VISIT_PARAMS << ";");
	});

	/* status arrays */
	PRINT_LINE("");
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		PRINT_LINE("\tstd::vector<NodeStatus> " << VISITED_ARR(&*s) << ";");
	});
}

void Printer::printInclusionImpl(std::ostream &fout, const NFA &lhs, const NFA &rhs,
				 int whichInclusion)
{
	auto ids = assignStateIDs(rhs.states_begin(), rhs.states_end());

	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		PRINT_LINE("void " << className << "::" << VISIT_PROC(&*s) << VISIT_PARAMS);
		PRINT_LINE("{");

		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("\tauto *lab = g.getEventLabel(" << "e" << ");");

		PRINT_LINE("");
		PRINT_LINE("\t" << VISITED_ARR(&*s) << "[lab->getStamp()]" << " = NodeStatus::entered;");
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(fout, "e", "p");
			PRINT_LINE("\t\tauto status = " << VISITED_IDX(t.dest, "p") << ";");
			PRINT_LINE("\t\tif (status == NodeStatus::unseen)");
			PRINT_LINE("\t\t\t" << VISIT_CALL(t.dest, "p") << ";");
			PRINT_LINE("\t}");
		});
		if (s->isStarting())
			PRINT_LINE("\t" << VISITED_ARR(&*s) << "[lab->getStamp()]" << " = NodeStatus::left;");
		PRINT_LINE("}");
		PRINT_LINE("");
	});

	PRINT_LINE("bool " << className << "::" << TOPLEVEL);
	PRINT_LINE("{");
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		PRINT_LINE("\t" << VISITED_ARR(&*s) << ".clear();");
		PRINT_LINE("\t" << VISITED_ARR(&*s) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);");
	});
	std::for_each(rhs.states_begin(), rhs.states_end(), [&](auto &s){
		if (s->isStarting())
			PRINT_LINE ("\t\t" << VISIT_CALL(&*s, "e") << ";");
	});
	/* TODO */
	PRINT_LINE("\treturn true;");
	PRINT_LINE("}");
	PRINT_LINE("");
}

#undef VISIT_PROC
#undef VISIT_CALL
#undef VISIT_PARAMS
#undef VISITED_ARR
#undef VISITED_IDX
#undef TOPLEVEL
