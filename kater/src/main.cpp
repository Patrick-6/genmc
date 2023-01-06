/*
 * KATER -- Automating Weak Memory Model Metatheory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-3.0.html.
 */

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

	ParsingDriver d;
	if (d.parse(config.inputFile))
		exit(EPARSE);
	if (config.verbose >= 1)
		std::cout << "Done.\n";

	Kater kater(config, d.takeModule());

	if (config.verbose >= 1)
		std::cout << "Checking assertions... ";
	if (!kater.checkAssertions())
		exit(ECHECK);
	if (config.verbose >= 1)
		std::cout << "Done.\n";

	if (config.generate) {
		if (config.verbose >= 1)
			std::cout << "Exporting code... ";
		if (!kater.exportCode(config.dir, config.name))
			exit(EPRINT);
		if (config.verbose >= 1)
			std::cout << "Done.\n";
	}

	return 0;
}
