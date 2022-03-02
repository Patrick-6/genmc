#include "Driver.hpp"
#include <iostream>
#include <fstream>

int Driver::parse ()
{
	if (config.verbose > 0)
		std::cout << "Parsing file " << config.inputFile << "...";
	location.initialize (&config.inputFile);
	scan_begin ();
	yy::parser parse (*this);
	if (config.debug) parse.set_debug_level(2);
	int res = parse ();
	scan_end ();
	if (config.verbose > 0)
		std::cout << "Done." << std::endl;
	return res;
}

void Driver::register_emptiness_assumption (std::unique_ptr<RegExp> e)
{
	auto p = has_trans_label(*e);
	if (!p.second) {
		std::cerr << "Warning: cannot use property " << *e << " = 0." << std::endl;
		return;
	}
	std::cout << "Registering property " << *e << " = 0." << std::endl;
	TransLabel::register_invalid(p.first);
}


void Driver::generate_NFAs ()
{
	for (int i = 0; i < saved_variables.size(); ++i) {
		if (config.verbose > 0)
			std::cout << "Generating NFA for save[" << i << "] = "
				  << saved_variables[i].first << std::endl;
		NFA n = saved_variables[i].first->toNFA();
		//if (p.second) n.optimize_for_transitive_reduction();
		n.simplify();
		if (config.verbose > 0)
			std::cout << "Generated NFA for save[" << i << "]: " << n << std::endl;
		nsaved.push_back({n, saved_variables[i].second});
	}

        for (auto &r : acyclicity_constraints) {
		if (config.verbose > 0)
			std::cout << "Generating NFA for acyclic " << *r << std::endl;
		// Covert the regural expression to an NFA
		NFA n = r->toNFA();
		// Take the reflexive-transitive closure, which typically helps minizing the NFA.
		// Doing so is alright because the generated DFS code discounts empty paths anyway.
		n.star();
		if (config.verbose > 1)
			std::cout << "Non-simplified NFA: " << n << std::endl;
		// Simplify the NFA
		n.simplify();
		if (config.verbose > 0 && acyclicity_constraints.size() > 1)
			std::cout << "Generated NFA: " << n << std::endl;
		nfa_acyc.alt(n);
        }
	if (config.verbose > 0)
		std::cout << "Generated NFA: " << nfa_acyc << std::endl;
}


static void printKaterNotice(const std::string &name, std::ostream &out = std::cout)
{
	out << "/* This file is generated automatically by Kater -- do not edit. */\n\n";
	return;
}


#define PRINT_LINE(line) fout << line << "\n"

void Driver::output_genmc_header_file ()
{
	std::string name = "Demo";
	std::string className = std::string("KaterConsChecker") + name;

	std::ofstream fout (className + ".hpp");
	if (!fout.is_open()) {
		return;
	}

	printKaterNotice(name, fout);

	PRINT_LINE("#ifndef __KATER_CONS_CHECKER_" << name << "_HPP__");
	PRINT_LINE("#define __KATER_CONS_CHECKER_" << name << "_HPP__");

	PRINT_LINE("");
	PRINT_LINE("#include \"ExecutionGraph.hpp\"");
	PRINT_LINE("#include \"EventLabel.hpp\"");
	PRINT_LINE("#include <bitset>");
	PRINT_LINE("#include <vector>");

	PRINT_LINE("");
	PRINT_LINE("class " << className << " {");
	PRINT_LINE("public:");

	for (int i = 0; i < nsaved.size(); ++i)
		nsaved[i].first.print_calculator_header_public(fout, i);
	nfa_acyc.print_acyclic_header_public(fout);

	PRINT_LINE("");
	PRINT_LINE("private:");
	PRINT_LINE("\tconst ExecutionGraph *g;");

	for (int i = 0; i < nsaved.size(); ++i)
		nsaved[i].first.print_calculator_header_private(fout, i);
	nfa_acyc.print_acyclic_header_private(fout);

	PRINT_LINE("};");

	PRINT_LINE("");
	PRINT_LINE("#endif /* __KATER_CONS_CHECKER_" << className << "_HPP__ */");
}

void Driver::output_genmc_impl_file ()
{
	std::string name = "Demo";
	std::string className = std::string("KaterConsChecker") + name;

	std::ofstream fout (className + ".cpp");
	if (!fout.is_open())
		return;

	printKaterNotice(name, fout);

	PRINT_LINE("#include \"" << className << ".hpp\"");
	PRINT_LINE("");

	for (int i = 0; i < nsaved.size(); ++i)
		nsaved[i].first.print_calculator_impl(fout, className, i, nsaved[i].second);
	nfa_acyc.print_acyclic_impl(fout, className);

}
