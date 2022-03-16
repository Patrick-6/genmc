#include "Config.hpp"
#include "Error.hpp"
#include "Printer.hpp"

template<typename ITER>
std::unordered_map<NFA::State *, unsigned> assignStateIDs(ITER &&begin, ITER &&end)
{
	std::unordered_map<NFA::State *, unsigned> result;

	auto i = 0u;
	std::for_each(begin, end, [&](auto &s){ result[&*s] = i++; });
	return result;
}

void openFileForWriting(const std::string &filename, std::ofstream &fout)
{
	fout.open(filename, std::ios_base::out);
	if (!fout.is_open()) {
		std::cerr << "Could not open file for writing: " << filename << "\n";
		exit(EPRINT);
	}
}

Printer::Printer(std::string name)
{
	/* Construct all the names to be used */
	std::transform(name.begin(), name.end(), std::back_inserter(prefix), ::toupper);
	className = prefix + "Checker";
	guardName = std::string("__") + prefix + "_CHECKER_HPP__";

	/* Open required streams */
	if (name != "") {
		openFileForWriting(className + ".hpp", foutHpp);
		outHpp = &foutHpp;
		openFileForWriting(className + ".cpp", foutCpp);
		outCpp = &foutCpp;
	}
}

void Printer::output(const NFAs *s)
{
	res = s;
	outputHeader();
	outputImpl();
}

static void printKaterNotice(std::ostream &out)
{
	out << "/* This file is generated automatically by Kater -- do not edit. */\n\n";
	return;
}

#define PRINT_LINE(line) (*outHpp) << line << "\n"

void Printer::outputHeader()
{
	printKaterNotice(*outHpp);

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
		printCalculatorHeader(*outHpp, res->nsaved[i].first, i);
	for (int i = 0; i < res->nincl.size(); ++i)
		printInclusionHeader(*outHpp, res->nincl[i].lhs, res->nincl[i].rhs, i);
	printAcyclicHeader(*outHpp, res->nfa_acyc);

	PRINT_LINE("\tExecutionGraph &g;");

	PRINT_LINE("");
	PRINT_LINE("\tExecutionGraph &getGraph() { return g; }");

	PRINT_LINE("};");

	PRINT_LINE("");
	PRINT_LINE("#endif /* " << guardName << " */");
}

#undef PRINT_LINE
#define PRINT_LINE(line) (*outCpp) << line << "\n"

void Printer::outputImpl()
{
	printKaterNotice(*outCpp);

	PRINT_LINE("#include \"" << className << ".hpp\"");
	PRINT_LINE("");

	for (int i = 0; i < res->nsaved.size(); ++i)
		printCalculatorImpl(*outCpp, res->nsaved[i].first, i, res->nsaved[i].second);
	for (int i = 0; i < res->nincl.size(); ++i)
		printInclusionImpl(*outCpp, res->nincl[i].lhs, res->nincl[i].rhs, i);
	printAcyclicImpl(*outCpp, res->nfa_acyc);
}

#undef PRINT_LINE
#define PRINT_LINE(line) fout << line << "\n"

/* --------------------------------------------------------------------- */
/*                          Calculators                                  */
/* --------------------------------------------------------------------- */

#define CALC             "calculate" << whichCalc << "(const Event &e)"
#define VISIT_PROC(i)    "visit" << whichCalc << "_" << ids[i]
#define VISIT_DECL(i)    VISIT_PROC(i) << "(VSet<Event> &calcRes, const Event &e)"
#define VISIT_CALL(i,e)  VISIT_PROC(i) << "(calcRes, " << e << ");"
#define VISITED_ARR(i)	 "visitedCalc" << whichCalc << "_" << ids[i]
#define VISITED_IDX(i,e) VISITED_ARR(i) << "[g.getEventLabel(" << e << ")->getStamp()]"

void Printer::printCalculatorHeader(std::ostream &fout, const NFA &nfa, int whichCalc)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* entry point */
	PRINT_LINE("\tVSet<Event> " << CALC << ";");
	PRINT_LINE("");

	/* visit procedures */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tvoid " << VISIT_DECL(&*s));
	});
	PRINT_LINE("");

	/* status arrays */
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tstd::vector<NodeStatus> " << VISITED_ARR(&*s) << ";");
	});
	PRINT_LINE("");
}

void Printer::printCalculatorImpl(std::ostream &fout, const NFA &nfa, int whichCalc, VarStatus reduce)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("void " << className << "::" << VISIT_DECL(&*s));
		PRINT_LINE("{");
		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("\tauto *lab = g.getEventLabel(" << "e" << ");");

		PRINT_LINE("");
		PRINT_LINE("\t" << VISITED_ARR(&*s) << "[lab->getStamp()]" << " = NodeStatus::entered;");
		if (s->isStarting()) {
			PRINT_LINE("\tcalcRes.insert(e);");
			if (reduce == VarStatus::Reduce) {
				PRINT_LINE("\tfor (const auto &p : calc" << whichCalc << "_preds(g, e)) {");
				PRINT_LINE("\t\tcalcRes.erase(p);");
				std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
					if (a->isAccepting())
						PRINT_LINE("\t\t" << VISITED_IDX(&*a, "p") << " = true;");
				});
				PRINT_LINE("\t}");
			}
		}
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(fout, "e", "p");
			PRINT_LINE("\t\tauto status = " << VISITED_IDX(t.dest, "p") << ";");
			PRINT_LINE("\t\tif (status == NodeStatus::unseen)");
			PRINT_LINE("\t\t\t" << VISIT_CALL(t.dest, "p"));
			PRINT_LINE("\t}");
		});
		PRINT_LINE("\t" << VISITED_ARR(&*s) << "[lab->getStamp()]" << " = NodeStatus::left;");
		PRINT_LINE("}");
		PRINT_LINE("");
	});

	PRINT_LINE("VSet<Event> " << className << "::" << CALC);
	PRINT_LINE("{");
	PRINT_LINE("\tVSet<Event> calcRes;");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\t" << VISITED_ARR(&*s) << ".clear();");
		PRINT_LINE("\t" << VISITED_ARR(&*s) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);");
	});
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &a){
		if (a->isAccepting())
			PRINT_LINE("\t" << VISIT_CALL(&*a, "e"));
	});
	PRINT_LINE("\treturn calcRes;");
	PRINT_LINE("}");
}

#undef CALC
#undef VISIT_PROC
#undef VISIT_DECL
#undef VISIT_CALL
#undef VISITED_ARR
#undef VISITED_IDX

/* --------------------------------------------------------------------- */
/*                      Acyclicity checker                               */
/* --------------------------------------------------------------------- */

#define VISIT_PROC(i)      "visitAcyclic" << ids[i]
#define VISIT_DECL(i)      VISIT_PROC(i) << "(const Event &e)"
#define VISIT_CALL(i,e)    VISIT_PROC(i) << "(" << e << ")"
#define VISITED_ARR(i)	   "visitedAcyclic" << ids[i]
#define VISITED_IDX(i,e)   VISITED_ARR(i) << "[g.getEventLabel(" << e << ")->getStamp()]"
#define VISITED_ACCEPTING  "visitedAccepting"
#define TOPLEVEL           "isAcyclic (const Event &e)"

void Printer::printAcyclicHeader(std::ostream &fout, const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	/* entry point */
	PRINT_LINE("");
	PRINT_LINE("\tbool " << TOPLEVEL << ";");

	/* visit procedures */
	PRINT_LINE("");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tbool " << VISIT_DECL(&*s) << ";");
	});

	/* status arrays */
	PRINT_LINE("");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\tstd::vector<NodeStatus> " << VISITED_ARR(&*s) << ";");
	});

	/* accepting counter */
	PRINT_LINE("");
	PRINT_LINE("\tint " << VISITED_ACCEPTING << ";");
}

void Printer::printAcyclicImpl(std::ostream &fout, const NFA &nfa)
{
	auto ids = assignStateIDs(nfa.states_begin(), nfa.states_end());

	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("bool " << className << "::" << VISIT_DECL(&*s));
		PRINT_LINE("{");

		PRINT_LINE("\tauto &g = getGraph();");
		PRINT_LINE("\tauto *lab = g.getEventLabel(" << "e" << ");");

		PRINT_LINE("");
		PRINT_LINE("\t" << VISITED_ARR(&*s) << "[lab->getStamp()]" << " = NodeStatus::entered;");
		if (s->isStarting())
			PRINT_LINE("\t++" << VISITED_ACCEPTING << ";");
		std::for_each(s->in_begin(), s->in_end(), [&](auto &t){
			t.label->output_for_genmc(fout, "e", "p");
			PRINT_LINE("\t\tauto status = " << VISITED_IDX(t.dest, "p") << ";");
			PRINT_LINE("\t\tif (status == NodeStatus::unseen && !" << VISIT_CALL(t.dest, "p") << ")");
			PRINT_LINE("\t\t\treturn false;");
			PRINT_LINE("\t\telse if (status == NodeStatus::entered && visitedAccepting)");
			PRINT_LINE("\t\t\treturn false;");
			PRINT_LINE("\t}");
		});
		if (s->isStarting())
			PRINT_LINE("\t--" << VISITED_ACCEPTING << ";");
		PRINT_LINE("\t" << VISITED_ARR(&*s) << "[lab->getStamp()]" << " = NodeStatus::left;");
		PRINT_LINE("\treturn true;");
		PRINT_LINE("}");
		PRINT_LINE("");
	});

	PRINT_LINE("bool " << className << "::" << TOPLEVEL);
	PRINT_LINE("{");
	PRINT_LINE("\t" << VISITED_ACCEPTING << " = 0;");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE("\t" << VISITED_ARR(&*s) << ".clear();");
		PRINT_LINE("\t" << VISITED_ARR(&*s) << ".resize(g.getMaxStamp() + 1, NodeStatus::unseen);");
	});
	PRINT_LINE("\treturn true");
	std::for_each(nfa.states_begin(), nfa.states_end(), [&](auto &s){
		PRINT_LINE ("\t\t&& " << VISIT_CALL(&*s, "e")
			    << (&*s == (--nfa.states_end())->get() ? ";" : ""));
	});
	PRINT_LINE("}");
	PRINT_LINE("");
}


#undef VISIT_PROC
#undef VISIT_DECL
#undef VISIT_CALL
#undef VISITED_ARR
#undef VISITED_IDX
#undef VISITED_ACCEPTING
#undef TOPLEVEL

/* --------------------------------------------------------------------- */
/*                      Inclusion checkers                               */
/* --------------------------------------------------------------------- */

#define VISIT_PROC(i)      "visitInclusion" << whichInclusion << "_" << ids[i]
#define VISIT_DECL(i)	   VISIT_PROC(i) << "(const Event &e)"
#define VISIT_CALL(i,e)    VISIT_PROC(i) << "(" << e << ")"
#define VISITED_ARR(i)	   "visitedInclusion" << whichInclusion << "_" << ids[i]
#define VISITED_IDX(i,e)   VISITED_ARR(i) << "[g.getEventLabel(" << e << ")->getStamp()]"
#define TOPLEVEL           "checkInclusion" << whichInclusion << "(const Event &e)"

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
		PRINT_LINE("\tvoid " << VISIT_DECL(&*s) << ";");
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
		PRINT_LINE("void " << className << "::" << VISIT_DECL(&*s));
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
#undef VISIT_DECL
#undef VISIT_CALL
#undef VISITED_ARR
#undef VISITED_IDX
#undef TOPLEVEL
