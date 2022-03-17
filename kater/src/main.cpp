#include "Config.hpp"
#include "Driver.hpp"
#include "Kater.hpp"
#include "Error.hpp"
#include "Printer.hpp"

#include <string.h>
#include <memory>

int main(int argc, char **argv)
{
	config.parseOptions(argc, argv);

	Driver d;

	if (d.parse())
		exit(EPARSE);

	d.checkAssertions();

	Kater kater(d.takeModule());

	kater.generateNFAs();
	kater.exportCode(config.outPrefix);

	return 0;
}
