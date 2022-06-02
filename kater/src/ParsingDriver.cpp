#include "ParsingDriver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

#define DEBUG_TYPE "parser"

extern FILE* yyin;

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
		yyin = s.in;
		location = s.loc;
		dir = s.dir;
		states.pop_back();
	}
}

int ParsingDriver::parse(const std::string &input)
{
	saveState();

	if (input.empty()) {
		std::cerr << "no input file provided" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (!(yyin = fopen((dir + input).c_str(), "r"))) {
		std::cerr << "cannot open " << input
			  << ": " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	auto s = input.find_last_of("/");
	dir = input.substr(0, s != std::string::npos ? s+1 : std::string::npos);

	location.initialize(&input);

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
