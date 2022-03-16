#ifndef KATER_DRIVER_HPP
#define KATER_DRIVER_HPP

#include "Config.hpp"
#include "NFA.hpp"
#include "Parser.hpp"
#include "TransLabel.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define YY_DECL yy::parser::symbol_type yylex (Driver& drv)
YY_DECL;

enum class VarStatus { Normal, Reduce };
enum class ConstraintType { Consistency, Error, Warning };

template<typename T>
struct Inclusion {
	T lhs;
	T rhs;
	ConstraintType type;
	const std::string   s;
};

// Results after conversion to NFA
struct NFAs {
	NFA nfa_acyc;
	std::vector<std::pair<NFA, VarStatus>> nsaved;
	std::vector<Inclusion<NFA>>            nincl;
};

class Driver {
public:
	using URE = std::unique_ptr<RegExp>;
	using UCO = std::unique_ptr<Constraint>;

	Driver();

	yy::location &getLocation() { return location; }

	void registerID(std::string id, URE re) {
		variables.insert({id, std::move(re)});
	}

	void registerSaveID(std::string id, URE re) {
		savedVariables.push_back({std::move(re), VarStatus::Normal});
		registerID(std::move(id), RelRE::create(RelLabel::getFreshCalcLabel()));
	}

	void registerSaveReducedID(std::string id, URE re) {
		savedVariables.push_back({std::move(re), VarStatus::Reduce});
		registerID(std::move(id), RelRE::create(RelLabel::getFreshCalcLabel()));
	}

	URE getRegisteredID(std::string id, const yy::location &loc) {
		auto it = variables.find(id);
		if (it == variables.end()) {
			std::cerr << loc << "\n";
			std::cerr << "Unknown relation encountered (" << id << ")\n";
			exit(EXIT_FAILURE);
		}
		return it->second->clone();
	}

	// Invoke the parser on the file `config->fileName`.  Return 0 on success.
	int parse ();

	// Check any user assertions and report errors.
	void checkAssertions();

	// Generate the NFAs for the regular expressions.
	void generate_NFAs(NFAs &res);

	// Handle "assert c" declaration in the input file
	void registerAssert(UCO c, const yy::location &loc) {
		asserts.push_back({std::move(c), loc});
	}

	// Handle "assume c" declaration in the input file
	void registerAssume(UCO c, const yy::location &loc);

	// Handle consistency constraint in the input file
	void addConstraint(UCO c, ConstraintType type, const std::string &s, const yy::location &loc);

private:
	using VarMap = std::unordered_map<std::string, URE>;
	using SavedVarSet = std::vector<std::pair<URE, VarStatus>>;

	void expandSavedVars(URE &r);

	// Location for lexing/parsing
	yy::location location;

	// Results from parsing the input file
	VarMap variables;

	// XXX: Maybe it's better to even have two containers below
	//      so that saved/reduced variables are treated differently,
	//      but I've no idea how many variable categories we want to have.
	//      If just two, I prefer separated. If more, polymorphism.
	SavedVarSet savedVariables;

	std::vector<std::pair<UCO, yy::location>> asserts;

	std::vector<URE>            acyclicityConstraints;
	std::vector<Inclusion<URE>> inclusionConstraints;
};

#endif /* KATER_DRIVER_HPP */
