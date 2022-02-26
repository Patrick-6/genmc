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

void Driver::register_emptiness_assumption (std::unique_ptr<RegExp> e)
{
	auto p = has_trans_label(*e);
	if (!p.second) {
		std::cerr << "Warning: cannot use property " << *e << " = 0." << std::endl;
		return;
	}
	std::cout << "Registering property " << *e << " = 0." << std::endl;
	TransLabel::register_invalid(p.first);
}
