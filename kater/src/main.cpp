#include "Config.hpp"
#include "ParsingDriver.hpp"
#include "Kater.hpp"
#include "Error.hpp"
#include "Printer.hpp"

#include <string.h>
#include <memory>

int main(int argc, char **argv)
{
	Config config;

	config.parseOptions(argc, argv);

	if (config.verbose >= 1)
		std::cout << "Parsing file " << config.inputFile << "... ";

	ParsingDriver d(config.inputFile);
	if (d.parse())
		exit(EPARSE);
	if (config.verbose >= 1)
		std::cout << "Done.\n";

	Kater kater(config, d.takeModule());

	if (config.verbose >= 1)
		std::cout << "Checking assertions... ";
	if (kater.checkAssertions())
		if (config.verbose >= 1)
			std::cout << "Done.\n";

	if (config.verbose >= 1)
		std::cout << "Exporting code... ";
	kater.exportCode(config.dirPrefix, config.outPrefix);
	if (config.verbose >= 1)
		std::cout << "Done.\n";

	return 0;
}
