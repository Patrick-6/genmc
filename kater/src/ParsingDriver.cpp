#include "ParsingDriver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

#define DEBUG_TYPE "parser"

extern FILE* yyin;
extern void yyrestart(FILE *);

ParsingDriver::ParsingDriver() : module(new KatModule)
{
	/* Basic predicates */
	std::for_each(Predicate::builtin_begin(), Predicate::builtin_end(), [this](auto &pi){
		registerBuiltinID(pi.second.name, CharRE::create(
					  TransLabel(std::nullopt,
						     Predicate::createBuiltin(pi.first))));
	});

	/* Basic relations */
	std::for_each(Relation::builtin_begin(), Relation::builtin_end(), [this](auto &ri){
		registerBuiltinID(ri.second.name, CharRE::create(
					  TransLabel(Relation::createBuiltin(ri.first))));
	});

	/* Default relations */
	registerBuiltinID("po", PlusRE::createOpt(module->getRegisteredID("po-imm")));
	registerBuiltinID("addr", PlusRE::createOpt(module->getRegisteredID("addr-imm")));
	registerBuiltinID("data", PlusRE::createOpt(module->getRegisteredID("data-imm")));
	registerBuiltinID("ctrl", SeqRE::createOpt(module->getRegisteredID("ctrl-imm"),
						   StarRE::createOpt(module->getRegisteredID("po-imm"))));
	registerBuiltinID("po-loc", PlusRE::createOpt(module->getRegisteredID("po-loc-imm")));
	registerBuiltinID("mo", PlusRE::createOpt(module->getRegisteredID("mo-imm")));
	registerBuiltinID("fr", SeqRE::createOpt(module->getRegisteredID("fr-imm"),
						 StarRE::createOpt(module->getRegisteredID("mo"))));
	registerBuiltinID("rmw", SeqRE::createOpt(module->getRegisteredID("UR"),
						  module->getRegisteredID("po-imm"),
						  module->getRegisteredID("UW")));
}

void ParsingDriver::saveState()
{
	states.push_back(State(getLocation(), yyin, dir, getPrefix()));
}

void ParsingDriver::restoreState()
{
	if (!states.empty()) {
		auto &s = states.back();
		yyrestart(s.in);
		location = s.loc;
		dir = s.dir;
		prefix = s.prefix;
		states.pop_back();
	}
}

int ParsingDriver::parse(const std::string &name)
{
	saveState();

	if (name.empty()) {
		std::cerr << "no input file provided" << std::endl;
		exit(EXIT_FAILURE);
	}

	auto path = dir + name;
	if (!(yyin = fopen(path.c_str(), "r"))) {
		std::cerr << "cannot open " << path
			  << ": " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	auto s = path.find_last_of("/");
	dir = path.substr(0, s != std::string::npos ? s+1 : std::string::npos);

	auto d = path.find_last_of(".");
	prefix = path.substr(s != std::string::npos ? s+1 : std::string::npos,
			     d != std::string::npos ? d-s-1 : std::string::npos);

	yyrestart(yyin);
	location.initialize(&path);

	yy::parser parser(*this);

	// KATER_DEBUG(
	// 	if (config.debug)
	// 		parser.set_debug_level(2);
	// );

	auto res = parser.parse();

	fclose(yyin);

	/* If @ top-level, save ppo */
	if (states.size() == 1)
		module->registerPPO(module->getRegisteredID(getQualifiedName("ppo")));

	restoreState();

	return res;
}
