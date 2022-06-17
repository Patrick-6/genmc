#ifndef __KATER_HPP__
#define __KATER_HPP__

#include "Config.hpp"
#include "CNFAs.hpp"
#include "KatModule.hpp"

/*
 * Given a KatModule resulted from parsing, transforms all regexps to
 * NFAs that will be used for code printing
 */
class Kater {

public:
	Kater() = delete;
	Kater(const Config &conf, std::unique_ptr<KatModule> mod)
		: config(conf), module(std::move(mod)) {}

	const KatModule &getModule() const { return *module; }
	KatModule &getModule() { return *module; }

	/* Check any user assertions and report errors.
	 * Returns whether any assertion was violated */
	bool checkAssertions();

	void generateNFAs();
	void exportCode(std::string &dirPrefix, std::string &outPrefix);

private:
	const Config &getConf() const { return config; }
	CNFAs &getCNFAs() { return cnfas; }

	void expandSavedVars(URE &r);
	void expandRfs(URE &r);

	const Config &config;
	std::unique_ptr<KatModule> module;

	CNFAs cnfas;
};

#endif /* __KATER_HPP__ */
