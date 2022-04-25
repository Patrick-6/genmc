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

	Kater kater(d.takeModule());

	kater.checkAssertions();
	kater.exportCode(config.dirPrefix, config.outPrefix);

	return 0;
}
