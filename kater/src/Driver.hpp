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

private:
	enum class VarStatus { Normal, Reduce };

public:
	yy::location &getLocation() { return location; }

	void registerID(std::string id, std::unique_ptr<RegExp> re) {
		variables.insert({id, std::move(re)});
	}

	void registerSaveID(std::string id, std::unique_ptr<RegExp> re) {
		savedVariables.push_back({std::move(re), VarStatus::Normal});
		registerID(std::move(id), CharRE::create(getFreshCalcID()));
	}

	void registerSaveReducedID(std::string id, std::unique_ptr<RegExp> re) {
		savedVariables.push_back({std::move(re), VarStatus::Reduce});
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

	// Handle "assume c" declaration in the input file
	void registerAssume(std::unique_ptr<Constraint> c, const yy::location &loc);

	// Handle consistency constraint in the input file
	void addConstraint(std::unique_ptr<Constraint> c, const yy::location &loc);

	// Output C++ files for GenMC
	void output_genmc_header_file();
	void output_genmc_impl_file();

private:
	using VarMap = std::unordered_map<std::string, std::unique_ptr<RegExp>>;
	using SavedVarSet = std::vector<std::pair<std::unique_ptr<RegExp>, VarStatus>>;

	std::string getFreshCalcID() const {
		return "calc" + std::to_string(savedVariables.size());
	}

	// Location for lexing/parsing
	yy::location location;

	// Results from parsing the input file
	VarMap variables;

	// XXX: Maybe it's better to even have two containers below
	//      so that saved/reduced variables are treated differently,
	//      but I've no idea how many variable categories we want to have.
	//      If just two, I prefer separated. If more, polymorphism.
	SavedVarSet savedVariables;

	std::vector<std::unique_ptr<RegExp> > acyclicityConstraints;

	// Results after conversion to NFA
	NFA nfa_acyc;
	std::vector<std::pair<NFA, VarStatus>> nsaved;
};

#endif /* KATER_DRIVER_HPP */
