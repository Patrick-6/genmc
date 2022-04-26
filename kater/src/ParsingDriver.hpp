#ifndef __PARSING_DRIVER_HPP__
#define __PARSING_DRIVER_HPP__

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

#define YY_DECL yy::parser::symbol_type yylex (ParsingDriver& drv)
YY_DECL;

class ParsingDriver {
public:
	using UCO = KatModule::UCO;

	ParsingDriver();

	yy::location &getLocation() { return location; }

	void registerID(std::string id, URE re) {
		module->registerID(std::move(id), std::move(re));
	}

	void registerSaveID(std::string idSave, std::string idRed, URE re, const yy::location &loc) {
		if (idRed != "" && idRed != idSave && !isAllowedReduction(idRed)) {
			std::cerr << loc << ": ";
			std::cerr << "forbidden reduction encountered \"" << idRed << "\"\n";
			exit(EXIT_FAILURE);
		}
		if (idRed != "") {
			module->registerSaveReducedID(idSave, idRed, std::move(re));
			auto redExp = module->getRegisteredID(idRed);
			auto saveExp = module->getRegisteredID(idSave);
			assert(&*redExp && &*saveExp);
			auto seqExp = SeqRE::createOpt(std::move(redExp), saveExp->clone());
			module->registerAssert(SubsetConstraint::createOpt(std::move(seqExp),
									   std::move(saveExp)), loc);
		} else
			module->registerSaveID(std::move(idSave), std::move(re));
	}

	void registerViewID(std::string id, URE re) {
		module->registerViewID(std::move(id), std::move(re));
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
		auto e = module->getRegisteredID(id);
		if (!e) {
			std::cerr << loc << ": ";
			std::cerr << "unknown relation encountered (" << id << ")\n";
			exit(EXIT_FAILURE);
		}
		return std::move(e);
	}

	// Invoke the parser on the file `config->fileName`.  Return 0 on success.
	int parse();

	std::unique_ptr<KatModule> takeModule() { return std::move(module); }

private:
	bool isAllowedReduction(const std::string &idRed) {
		return idRed == "po" || idRed == "po-loc" || idRed == "po-imm";
	}

	// Location for lexing/parsing
	yy::location location;

	std::unique_ptr<KatModule> module;
};

#endif /* __PARSING_DRIVER_HPP__ */
