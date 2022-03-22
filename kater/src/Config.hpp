#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <string>

struct Config {

	/* Initializes the configuration based on the CLI arguments ARGC/ARGV */
	void parseOptions(int argc, char **argv);

	bool debug;
        int verbose;
	std::string dirPrefix;
	std::string outPrefix;
	std::string inputFile;

private:
	/* Resets config to its default values */
	void reset();

	/* Prints usage instructions if config parsing fails */
	void printUsage(const char *kater);
};

extern Config config;

#endif /* __CONFIG_HPP__ */
