#include "Driver.hpp"
#include "Config.hpp"
#include "Error.hpp"

#include <string.h>
#include <memory>

int main(int argc, char **argv)
{
	auto config = std::make_unique<Config>();

	config->parseOptions(argc, argv);

	Driver d(config->debug);

	std::cout << "Parsing file " << config->inputFile << "...";
	if (d.parse(config->inputFile))
		exit(EPARSE);
	std::cout << " Done.\n";

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
