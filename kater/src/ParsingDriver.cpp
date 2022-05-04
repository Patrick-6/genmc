#include "ParsingDriver.hpp"
#include <cstring>
#include <iostream>
#include <fstream>

#define DEBUG_TYPE "parser"

ParsingDriver::ParsingDriver(const std::string &input)
	: file(input), module(new KatModule)
{
	auto i = 0u;
	std::for_each(PredLabel::builtin_begin(), PredLabel::builtin_end(), [&i,this](auto &pi){
		registerID(pi.name, PredRE::create(i++));
	});

	i = 0u;
	std::for_each(RelLabel::builtin_begin(), RelLabel::builtin_end(), [&i,this](auto &ri){
		registerID(ri.name, RelRE::create(i++));
	});
}

int ParsingDriver::parse()
{
	extern FILE* yyin;

	if (getFile().empty()) {
		std::cerr << "no input file provided" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (!(yyin = fopen(getFile().c_str(), "r"))) {
		std::cerr << "cannot open " << getFile()
			  << ": " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	location.initialize(&getFile());

	yy::parser parser(*this);

	// KATER_DEBUG(
	// 	if (config.debug)
	// 		parser.set_debug_level(2);
	// );

	auto res = parser.parse();

	fclose(yyin);

	return res;
}
