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

#ifndef KATER_KATER_HPP
#define KATER_KATER_HPP

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
	void registerDefaultAssumptions();
	bool checkAssertion(const Constraint *c, Counterexample &cex);

	void printCounterexample(const Counterexample &cex) const;

	const Config &config;
	std::unique_ptr<KatModule> module;

	CNFAs cnfas;
};

#endif /* KATER_KATER_HPP */
