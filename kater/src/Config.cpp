#include <config.h>
#include "Config.hpp"
#include "Error.hpp"
#include <iostream>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

void Config::reset()
{
	verbose = 0;
	debug = false;
	debugOnly = "";
	outPrefix = "";
	dirPrefix = "/tmp";
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
#ifdef ENABLE_KATER_DEBUG
"-d, --debug                 Print all debugging information.\n"
"                            Default: %d\n"
"--debug-only                Print debugging information of the specified type(s).\n"
"                            Default: \"%s\"\n"
#endif
"-o, --output                Name prefix to be used for the resulting files.\n"
"                            Default: \"%s\" (prints to stdout)\n"
"-p, --prefix                Directory where the resulting files will be stored.\n"
"                            Has no effect without -o.\n"
"                            Default: \"%s\"\n"
"-v[NUM], --verbose[=NUM]    Print verbose execution information. NUM is optional:\n"
"                            0 is quiet; 1 prints status; 2 is noisy;\n"
"                            3 is noisier.\n"
"                            Default: %d\n",
		kater,
#ifdef ENABLE_KATER_DEBUG
		debug,
		debugOnly.c_str(),
#endif
		outPrefix.c_str(),
		dirPrefix.c_str(),
		verbose);
	exit(0);
}

void Config::parseOptions(int argc, char **argv)
{
	/* Reset defaults before parsing the options */
	reset();

#ifdef ENABLE_KATER_DEBUG
#define DEBUG_ONLY_OPT 4242
#endif

	const char *shortopts = "hdp:o:v::";
	const struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
#ifdef ENABLE_KATER_DEBUG
		{"debug", no_argument, NULL, 'd'},
		{"debug-only", required_argument, NULL, DEBUG_ONLY_OPT},
#endif
		{"output", required_argument, NULL, 'o'},
		{"prefix", required_argument, NULL, 'p'},
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
		case 'o':
			outPrefix = optarg;
			break;
		case 'p':
			dirPrefix = optarg;
			break;
		case 'v':
			verbose = optarg ? atoi(optarg) : 1;
			break;
#ifdef ENABLE_KATER_DEBUG
		case 'd':
			debug = true;
			katerDebug = true;
			break;
		case DEBUG_ONLY_OPT:
			debugOnly = debugOnly.empty() ? optarg : (debugOnly + optarg);
			addDebugType(optarg);
			break;
#endif
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
