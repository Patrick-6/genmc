#include "Driver.hpp"
#include "Config.hpp"
#include "Error.hpp"

#include <string.h>
#include <memory>

int main(int argc, char **argv)
{
	Driver d;
	d.config.parseOptions(argc, argv);

	if (d.parse())
		exit(EPARSE);

	d.generate_NFAs();

	d.output_genmc_header_file();
	d.output_genmc_impl_file();
	return 0;
}
