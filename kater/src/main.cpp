#include "Config.hpp"
#include "Driver.hpp"
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

	NFAs res;
	d.generate_NFAs(res);

	Printer p (&res);
	p.output();

	return 0;
}
