#include "Config.hpp"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

Config config;

void Config::reset()
{
	verbose = 0;
	debug = false;
	inputFile = "";
}

void Config::printUsage(const char *kater)
{
	/* Reset defaults before printing */
	reset();

	printf(
"OVERVIEW: kater -- consistency check routine generator\n"
"Usage: %s [options] <input file>\n"
"\n"
"OPTIONS:\n"
"\n"
"-h, --help                  Display this help message and exit\n"
"-d, --debug                 Print debugging information.\n"
"                            Default: %d\n"
"-v[NUM], --verbose[=NUM]    Print verbose execution information. NUM is optional:\n"
"                              0 is quiet; 1 prints status; 2 is noisy;\n"
"                              3 is noisier.\n"
"                              Default: %d\n",
		kater,
		debug,
		verbose);
	exit(0);
}

void Config::parseOptions(int argc, char **argv)
{
	/* Reset defaults before parsing the options */
	reset();

	const char *shortopts = "hdv:";
	const struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"debug", no_argument, NULL, 'd'},
		{"verbose", optional_argument, NULL, 'v'},
		{0, 0, 0, 0} /* Terminator */
	};

	int opt, longindex;
	bool error = false;
	while (!error && (opt = getopt_long(argc, argv, shortopts, longopts, &longindex)) != -1) {
		switch (opt) {
		case 'h':
			printUsage(argv[0]);
			break;
		case 'd':
			debug = true;
			break;
		case 'v':
			verbose = optarg ? atoi(optarg) : 1;
			break;
		default: /* '?' */
			error = true;
			break;
		}
	}

	/* If there is a remaining argument, set the filename accordingly */
	if (argc > optind)
		inputFile = *(argv + optind);

	if (error)
		printUsage(argv[0]);
}
