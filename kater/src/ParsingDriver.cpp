#include "ParsingDriver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

#define DEBUG_TYPE "parser"

extern FILE* yyin;
extern void yyrestart(FILE *);

ParsingDriver::ParsingDriver() : module(new KatModule)
{
	auto i = 0;
	std::for_each(TransLabel::builtin_pred_begin(), TransLabel::builtin_pred_end(), [&i,this](auto &pi){
		registerID(pi.name, CharRE::create(TransLabel(std::nullopt, {i++})));
	});

	i = 0;
	std::for_each(TransLabel::builtin_rel_begin(), TransLabel::builtin_rel_end(), [&i,this](auto &ri){
		registerID(ri.name, CharRE::create(TransLabel(i++)));
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
