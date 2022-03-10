#include "Driver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

const std::unordered_set<std::string> Driver::builtinNames = {
	"po_imm",
	"mo_imm", "mo_imm_int", "mo_imm_ext",
	"fr_init", "fr_init_int", "fr_init_ext",
	"rf", "rf_int", "rf_ext",
	"rf_inv", "rf_inv_int", "rf_inv_ext",
	"rmw",
	"ctrl_imm", "addr_imm", "data_imm",
	"W", "R", "F",
	"RLX", "ACQ", "REL", "SC",
};

int Driver::parse()
{
	extern FILE* yyin;

	if (config.verbose > 0)
		std::cout << "Parsing file " << config.inputFile << "...";

	if (config.inputFile.empty ()) {
		std::cerr << "no input file provided" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (!(yyin = fopen(config.inputFile.c_str(), "r"))) {
		std::cerr << "cannot open " << config.inputFile
			  << ": " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	location.initialize(&config.inputFile);

	yy::parser parser(*this);
	if (config.debug)
		parser.set_debug_level(2);

	auto res = parser.parse();

	fclose(yyin);

	if (config.verbose > 0)
		std::cout << "Done." << std::endl;
	return res;
}


void Driver::registerAssume(std::unique_ptr<Constraint> c, const yy::location &loc)
{
	if (auto *empC = dynamic_cast<EmptyConstraint *>(&*c)) {
		auto *charRE = dynamic_cast<const CharRE *>(empC->getKid(0));
		if (!charRE) {
			std::cerr << loc << ": [Warning] Ignoring the unsupported assumption "
				  << *empC->getKid(0) << std::endl;
			return;
		}
		std::cout << "Registering assumption " << *empC->getKid(0) << "." << std::endl;
		TransLabel::register_invalid(charRE->getLabel());
	}
}

void Driver::addConstraint(std::unique_ptr<Constraint> c, const yy::location &loc)
{
	if (auto *acycC = dynamic_cast<AcyclicConstraint *>(&*c)) {
		acyclicityConstraints.push_back(acycC->getKid(0)->clone());
		return;
	}
	std::cerr << loc << ": [Warning] Ignoring the unsupported constraint " << *c << std::endl;
}

void Driver::generate_NFAs ()
{
	for (auto i = 0u; i < savedVariables.size(); i++) {
		auto &v = savedVariables[i];
		if (config.verbose > 0)
			std::cout << "Generating NFA for save[" << i << "] = "
				  << *v.first << std::endl;
		NFA n = v.first->toNFA();
		if (v.second == VarStatus::Reduce)
			n.simplifyReduce();
		else
			n.simplify();
		if (config.verbose > 0)
			std::cout << "Generated NFA for save[" << i << "]: " << n << std::endl;
		nsaved.push_back({std::move(n), v.second});
	}

        for (auto &r : acyclicityConstraints) {
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
		if (config.verbose > 0 && acyclicityConstraints.size() > 1)
			std::cout << "Generated NFA: " << n << std::endl;
		nfa_acyc.alt(std::move(n));
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
	PRINT_LINE("#include \"VSet.hpp\"");
	PRINT_LINE("#include <vector>");

	PRINT_LINE("");
	PRINT_LINE("class " << className << " {");
	PRINT_LINE("public:");

	for (int i = 0; i < nsaved.size(); ++i)
		nsaved[i].first.print_calculator_header_public(fout, i);
	nfa_acyc.print_acyclic_header_public(fout);

	PRINT_LINE("");
	PRINT_LINE("private:");
	PRINT_LINE("\tconst ExecutionGraph &g;");

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

	for (int i = 0; i < nsaved.size(); ++i) {
		if (nsaved[i].second == VarStatus::Reduce)
			nsaved[i].first.printCalculatorImplReduce(fout, className, i);
		else
			nsaved[i].first.printCalculatorImpl(fout, className, i);
	}
	nfa_acyc.print_acyclic_impl(fout, className);

}
