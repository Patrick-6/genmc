#ifndef KATER_DRIVER_HPP
#define KATER_DRIVER_HPP

#include "Config.hpp"
#include "KatModule.hpp"
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

class Driver {
public:
	using URE = KatModule::URE;
	using UCO = KatModule::UCO;

	Driver();

	yy::location &getLocation() { return location; }

	void registerID(std::string id, URE re) {
		module->registerID(std::move(id), std::move(re));
	}

	void registerSaveID(std::string id, URE re) {
		module->registerSaveID(std::move(id), std::move(re));
	}

	void registerSaveReducedID(std::string id, URE re) {
		module->registerSaveReducedID(std::move(id), std::move(re));
	}

	// Handle "assert c" declaration in the input file
	void registerAssert(UCO c, const yy::location &loc) {
		module->registerAssert(std::move(c), loc);
	}

	// Handle "assume c" declaration in the input file
	void registerAssume(UCO c, const yy::location &loc) {
		module->registerAssume(std::move(c), loc);
	}

	// Handle consistency constraint in the input file
	void addConstraint(UCO c, const std::string &s, const yy::location &loc) {
		module->addConstraint(std::move(c), s, loc);
	}

	URE getRegisteredID(std::string id, const yy::location &loc) {
		auto e = module->getRegisteredID(id, loc);
		if (!e) {
			std::cerr << loc << "\n";
			std::cerr << "Unknown relation encountered (" << id << ")\n";
			exit(EXIT_FAILURE);
		}
		return std::move(e);
	}

	// Invoke the parser on the file `config->fileName`.  Return 0 on success.
	int parse();

	// Check any user assertions and report errors.
	void checkAssertions();

	std::unique_ptr<KatModule> takeModule() { return std::move(module); }

private:
	void expandSavedVars(URE &r);

	// Location for lexing/parsing
	yy::location location;

	std::unique_ptr<KatModule> module;
};

#endif /* KATER_DRIVER_HPP */
