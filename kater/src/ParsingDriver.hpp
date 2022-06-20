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

private:
	struct State {
		State(yy::location loc, FILE* in,
		      const std::string &dir, const std::string &prefix)
			: loc(loc), in(in), dir(dir), prefix(prefix) {}

		yy::location loc;
		FILE* in;
		std::string dir;
		std::string prefix;
	};

public:
	using UCO = KatModule::UCO;

	ParsingDriver();

	yy::location &getLocation() { return location; }

	const std::string &getPrefix() const { return prefix; }

	std::string getQualifiedName(const std::string &id) const {
		return getPrefix() + "::" + id;
	}
	std::string getUnqualifiedName(const std::string &id) const {
		auto c = id.find_last_of(":");
		return id.substr(c != std::string::npos ? c+1 : c, std::string::npos);
	}

	void registerBuiltinID(std::string id, URE re) {
		module->registerID(id, std::move(re));
	}

	void registerID(std::string id, URE re) {
		module->registerID(getQualifiedName(id), std::move(re));
	}

	void registerSaveID(std::string idSave, std::string idRed, URE re, const yy::location &loc) {
		if (idRed != "" && idRed != idSave && !isAllowedReduction(idRed)) {
			std::cerr << loc << ": ";
			std::cerr << "forbidden reduction encountered \"" << idRed << "\"\n";
			exit(EXIT_FAILURE);
		}
		if (idRed != "") {
			std::string rname;
			if (idRed == idSave || module->getRegisteredID(getQualifiedName(idRed)))
				rname = getQualifiedName(idRed);
			else
				rname = idRed;
			if (idRed != idSave)
				getRegisteredID(rname, loc); // ensure exists

			module->registerSaveReducedID(getQualifiedName(idSave), rname, std::move(re));
		} else {
			module->registerSaveID(getQualifiedName(idSave), std::move(re));
		}
	}

	void registerViewID(std::string id, URE re) {
		module->registerViewID(getQualifiedName(id), std::move(re));
	}

	// Handle "assert c" declaration in the input file
	void registerAssert(UCO c, const yy::location &loc) {
		module->registerAssert(std::move(c), loc);
	}

	// Handle "assume c" declaration in the input file
	void registerAssume(UCO c, const yy::location &loc) {
		module->registerAssume(std::move(c));
	}

	// Handle consistency constraint in the input file
	void addConstraint(UCO c, const std::string &s, const yy::location &loc) {
		module->addConstraint(std::move(c), s, loc);
	}

	URE getRegisteredID(std::string id, const yy::location &loc) {
		auto e = module->getRegisteredID(getQualifiedName(id));
		if (!e) {
			auto f = module->getRegisteredID(id);
			if (!f) {
				std::cerr << loc << ": ";
				std::cerr << "unknown relation encountered (" << id << ")\n";
				exit(EXIT_FAILURE);
			}
			e = std::move(f);
		}
		return std::move(e);
	}

	/* Invoke the parser on INPUT. Return 0 on success. */
	int parse(const std::string &input);

	std::unique_ptr<KatModule> takeModule() { return std::move(module); }

private:
	bool isAllowedReduction(const std::string &idRed) {
		auto id = getUnqualifiedName(idRed);
		return id == "po" || id == "po-loc" || id == "po-imm";
	}

	void saveState();
	void restoreState();

	/* Location for lexing/parsing */
	yy::location location;

	/* Current source file directory (used for includes) */
	std::string dir;

	/* Current name prefix */
	std::string prefix;

	std::unique_ptr<KatModule> module;

	std::vector<State> states;
};

#endif /* __PARSING_DRIVER_HPP__ */
