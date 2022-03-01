#ifndef DRIVER_HPP
# define DRIVER_HPP
# include <memory>
# include <string>
# include <unordered_map>
# include "Parser.hpp"

# define YY_DECL yy::parser::symbol_type yylex (Driver& drv)
YY_DECL;

class Driver {

public:
	Driver(bool debug = false) : variables(), acyclicity_constraints(),
				     result(), debug(debug), file(),
				     location() {}

	std::unordered_map<std::string, std::unique_ptr<RegExp> > variables;
	std::vector<std::unique_ptr<RegExp> > acyclicity_constraints;

	int result;
	bool debug;

	// Run the parser on file F.  Return 0 on success.
	int parse (const std::string& f);

	void register_emptiness_assumption(std::unique_ptr<RegExp> e);

	std::string file;

	void scan_begin();
	void scan_end();
	yy::location location;
};

#endif /* DRIVER_HPP */
