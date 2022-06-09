#include "ParsingDriver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

#define DEBUG_TYPE "parser"

extern FILE* yyin;
extern void yyrestart(FILE *);

ParsingDriver::ParsingDriver() : module(new KatModule)
{
	std::for_each(Predicate::builtin_begin(), Predicate::builtin_end(), [this](auto &pi){
		registerID(pi.second.name, CharRE::create(
				   TransLabel(std::nullopt, Predicate::createBuiltin(pi.first))));
	});
	std::for_each(Relation::builtin_begin(), Relation::builtin_end(), [this](auto &ri){
		registerID(ri.second.name, CharRE::create(
				   TransLabel(Relation::createBuiltin(ri.first))));
	});
}

void ParsingDriver::saveState()
{
	states.push_back(State(getLocation(), yyin, dir));
}

void ParsingDriver::restoreState()
{
	if (!states.empty()) {
		auto &s = states.back();
		yyrestart(s.in);
		location = s.loc;
		dir = s.dir;
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

	yyrestart(yyin);
	location.initialize(&path);

	yy::parser parser(*this);

	// KATER_DEBUG(
	// 	if (config.debug)
	// 		parser.set_debug_level(2);
	// );

	auto res = parser.parse();

	fclose(yyin);

	restoreState();

	return res;
}
