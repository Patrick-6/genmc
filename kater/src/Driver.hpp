#ifndef KATER_DRIVER_HPP
#define KATER_DRIVER_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "Config.hpp"
#include "NFA.hpp"
#include "Parser.hpp"

#define YY_DECL yy::parser::symbol_type yylex (Driver& drv)
YY_DECL;

using string_RE_map = std::unordered_map<std::string, std::unique_ptr<RegExp>>;

class Driver {

public:
	// Location for lexing/parsing
	yy::location location;

	// Results from parsing the input file
	string_RE_map variables;
	std::vector<std::pair<std::unique_ptr<RegExp>, bool>> saved_variables;
	std::vector<std::unique_ptr<RegExp> > acyclicity_constraints;

	// Results after conversion to NFA
	NFA nfa_acyc;
	std::vector<std::pair<NFA, bool>> nsaved;

	//---------------------------------------------------------

	// Invoke the parser on the file `config->fileName`.  Return 0 on success.
	int parse ();

	// Generate the NFAs for the regular expressions.
	void generate_NFAs ();

	// Handle "property e = 0" declaration in the input file
	void register_emptiness_assumption(std::unique_ptr<RegExp> e);

	// Output C++ files for GenMC
	void output_genmc_header_file();
	void output_genmc_impl_file();
};

#endif /* KATER_DRIVER_HPP */
