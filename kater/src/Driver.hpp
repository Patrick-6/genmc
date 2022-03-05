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

class Driver {

public:
	yy::location &getLocation() { return location; }

	void addAcyclicityConstraint(std::unique_ptr<RegExp> re) {
		acyclicityConstraints.push_back(std::move(re));
	}

	void registerID(std::string id, std::unique_ptr<RegExp> re) {
		variables.insert({id, std::move(re)});
	}

	void registerSaveID(std::string id, std::unique_ptr<RegExp> re) {
		savedVariables.push_back({std::move(re), false});
		registerID(std::move(id), CharRE::create(getFreshCalcID()));
	}

	void registerSaveReducedID(std::string id, std::unique_ptr<RegExp> re) {
		savedVariables.push_back({std::move(re), true});
		registerID(std::move(id), CharRE::create(getFreshCalcID()));
	}

	std::unique_ptr<RegExp> createIDOrGetRegistered(std::string id) {
		auto it = variables.find(id);
		return (it == variables.end()) ? CharRE::create(id) : it->second->clone();
	}

	// Invoke the parser on the file `config->fileName`.  Return 0 on success.
	int parse ();

	// Generate the NFAs for the regular expressions.
	void generate_NFAs ();

	// Handle "property e = 0" declaration in the input file
	void register_emptiness_assumption(std::unique_ptr<RegExp> e);

	// Output C++ files for GenMC
	void output_genmc_header_file();
	void output_genmc_impl_file();

private:
	using VarMap = std::unordered_map<std::string, std::unique_ptr<RegExp>>;

	std::string getFreshCalcID() const {
		return "calc" + std::to_string(savedVariables.size());
	}

	// Location for lexing/parsing
	yy::location location;

	// Results from parsing the input file
	VarMap variables;
	std::vector<std::pair<std::unique_ptr<RegExp>, bool>> savedVariables;
	std::vector<std::unique_ptr<RegExp> > acyclicityConstraints;

	// Results after conversion to NFA
	NFA nfa_acyc;
	std::vector<std::pair<NFA, bool>> nsaved;
};

#endif /* KATER_DRIVER_HPP */
