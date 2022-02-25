#include "Driver.hpp"

int Driver::parse (const std::string &f)
{
	file = f;
	location.initialize (&file);
	scan_begin ();
	yy::parser parse (*this);
	if (debug) parse.set_debug_level(2);
	int res = parse ();
	scan_end ();
	return res;
}
