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

#ifndef KATER_CNFAS_HPP
#define KATER_CNFAS_HPP

#include "Inclusion.hpp"
#include "KatModuleAPI.hpp"
#include "NFA.hpp"

/*
 * Constrained NFAs class
 * A collection of NFAs representing different constraint types
 */
class CNFAs {

public:
	CNFAs() = default;

	using save_iter = std::vector<std::pair<NFA, VarStatus>>::iterator;
	using save_const_iter = std::vector<std::pair<NFA, VarStatus>>::const_iterator;
	using redc_iter = save_iter;
	using redc_const_iter = save_const_iter;
	using incl_iter = std::vector<Inclusion<NFA>>::iterator;
	using incl_const_iter = std::vector<Inclusion<NFA>>::const_iterator;

	const NFA &getAcyclic() const { return acyc; }

	const NFA &getRecovery() const { return rec; }

	const std::pair<NFA, bool> &getPPoRf() const { return pporf; }

	save_iter save_begin() { return nsave.begin(); }
	save_iter save_end() { return nsave.end(); }
	save_const_iter save_begin() const { return nsave.begin(); }
	save_const_iter save_end() const { return nsave.end(); }

	// redc_iter redc_begin() { return nredc.begin(); }
	// redc_iter redc_end() { return nredc.end(); }
	// redc_const_iter redc_begin() const { return nredc.begin(); }
	// redc_const_iter redc_end() const { return nredc.end(); }

	incl_iter incl_begin() { return nincl.begin(); }
	incl_iter incl_end() { return nincl.end(); }
	incl_const_iter incl_begin() const { return nincl.begin(); }
	incl_const_iter incl_end() const { return nincl.end(); }

	void addAcyclic(NFA &&a) { acyc.alt(std::move(a)); }

	void addRecovery(NFA &&a) { rec.alt(std::move(a)); }

	void addPPoRf(NFA &&ppo, bool deps = false) { pporf = std::make_pair(std::move(ppo), deps); }

	void addSaved(NFA &&save) { nsave.push_back({std::move(save), VarStatus::Normal}); }

	void addReduced(NFA &&redc) { nsave.push_back({std::move(redc), VarStatus::Reduce}); }

	void addView(NFA &&view) { nsave.push_back({std::move(view), VarStatus::View}); }

	void addInclusion(Inclusion<NFA> &&incl) { nincl.push_back(std::move(incl)); }

private:
	NFA acyc;
	NFA rec;
	std::pair<NFA, bool> pporf; // 1 -> deps
	std::vector<std::pair<NFA, VarStatus>> nsave;
	// std::vector<NFA> nredc;
	std::vector<Inclusion<NFA>> nincl;
};

#endif /* KATER_CNFAS_HPP */
