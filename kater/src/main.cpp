#include "Driver.hpp"
#include <cstring>

int main (int argc, const char *argv[])
{
	Driver d;
	std::string s ("");
	if (argc > 1)
		s = argv[argc - 1];
	d.debug = (argc > 1 && strcmp(argv[1], "-p") == 0);
	std::cout << "Parsing file " << s << "..." << std::endl;
	d.parse (s);
	std::cout << "Parsed input file" << std::endl;

	NFA f;
	for (auto &r : d.acyclicity_constraints) {
		std::cout << "Generating NFA for " << *r << std::endl;
		// Covert the regural expression to an NFA
		NFA n = r->toNFA();
		// Take the reflexive-transitive closure, which typically helps minizing the NFA.
		// Doing so is alright because the generated DFS code discounts empty paths anyway.
		n.star();
		std::cout << "Non-simplified generated NFA: " << n << std::endl;
		// Simplify the NFA
		n.simplify();
		if (d.acyclicity_constraints.size() > 1)
			std::cout << "Generated NFA: " << n << std::endl;
		f.alt(n);
	}
	std::cout << "Final generated NFA: " << f << std::endl;

	f.print_visitors_header_file("Demo");
	f.print_visitors_impl_file("Demo");
	return 0;
}
