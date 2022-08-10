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

#ifndef KATER_KAT_MODULE_HPP
#define KATER_KAT_MODULE_HPP

#include "Inclusion.hpp"
#include "KatModuleAPI.hpp"
#include "Parser.hpp"

class KatModule {

public:
	using UCO = std::unique_ptr<Constraint>;

	struct DbgInfo {
		DbgInfo(const std::string *name, int l) : filename(name != nullptr ? *name : ""), line(l) {}
		std::string filename;
		int line;

		friend std::ostream &operator<<(std::ostream &s, const DbgInfo &dbg);
	};

private:
	using VarMap = std::unordered_map<std::string, URE>;
	using SavedVarMap = std::vector<std::pair<Relation, SavedVar>>;

	using Assert = struct {
		UCO co;
		DbgInfo loc;
	};

public:

	KatModule() = default;

	using var_iter = VarMap::iterator;
	using var_const_iter = VarMap::const_iterator;
	using svar_iter = SavedVarMap::iterator;
	using svar_const_iter = SavedVarMap::const_iterator;
	using acyc_iter = std::vector<URE>::iterator;
	using acyc_const_iter = std::vector<URE>::const_iterator;
	using rec_iter = std::vector<URE>::iterator;
	using rec_const_iter = std::vector<URE>::const_iterator;
	using incl_iter = std::vector<Inclusion<URE>>::iterator;
	using incl_const_iter = std::vector<Inclusion<URE>>::const_iterator;
	using assr_iter = std::vector<Assert>::iterator;
	using assr_const_iter = std::vector<Assert>::const_iterator;
	using assm_iter = std::vector<UCO>::iterator;
	using assm_const_iter = std::vector<UCO>::const_iterator;

	var_iter var_begin() { return variables.begin(); }
	var_iter var_end() { return variables.end(); }
	var_const_iter var_begin() const { return variables.begin(); }
	var_const_iter var_end() const { return variables.end(); }

	svar_iter svar_begin() { return savedVariables.begin(); }
	svar_iter svar_end() { return savedVariables.end(); }
	svar_const_iter svar_begin() const { return savedVariables.begin(); }
	svar_const_iter svar_end() const { return savedVariables.end(); }

	acyc_iter acyc_begin() { return acyclicityConstraints.begin(); }
	acyc_iter acyc_end() { return acyclicityConstraints.end(); }
	acyc_const_iter acyc_begin() const { return acyclicityConstraints.begin(); }
	acyc_const_iter acyc_end() const { return acyclicityConstraints.end(); }

	rec_iter rec_begin() { return recoveryConstraints.begin(); }
	rec_iter rec_end() { return recoveryConstraints.end(); }
	rec_const_iter rec_begin() const { return recoveryConstraints.begin(); }
	rec_const_iter rec_end() const { return recoveryConstraints.end(); }

	incl_iter incl_begin() { return inclusionConstraints.begin(); }
	incl_iter incl_end() { return inclusionConstraints.end(); }
	incl_const_iter incl_begin() const { return inclusionConstraints.begin(); }
	incl_const_iter incl_end() const { return inclusionConstraints.end(); }

	assr_iter assert_begin() { return asserts.begin(); }
	assr_iter assert_end() { return asserts.end(); }
	assr_const_iter assert_begin() const { return asserts.begin(); }
	assr_const_iter assert_end() const { return asserts.end(); }

	assm_iter assume_begin() { return assumes.begin(); }
	assm_iter assume_end() { return assumes.end(); }
	assm_const_iter assume_begin() const { return assumes.begin(); }
	assm_const_iter assume_end() const { return assumes.end(); }

	size_t getRegisteredNum() const { return variables.size(); }

	size_t getSavedNum() const { return savedVariables.size(); }

	size_t getAssertNum() const { return asserts.size(); }

	size_t getAssumeNum() const { return assumes.size(); }

	size_t getAcyclicNum() const { return acyclicityConstraints.size(); }

	size_t getRecoveryNum() const { return recoveryConstraints.size(); }

	size_t getInclusionNum() const { return inclusionConstraints.size(); }

	URE getPPO() const { return ppo->clone(); }

	const std::vector<UCO> &getAssumes() const { return assumes; }

	void registerID(std::string id, URE re) {
		variables.insert({id, std::move(re)});
	}

	void registerSaveID(std::string id, URE re) {
		auto r = Relation::createUser();
		registerID(std::move(id), CharRE::create(TransLabel(r)));
		savedVariables.push_back({r, SavedVar(std::move(re))});
	}

	void registerSaveReducedID(std::string idSave, std::string idRed, URE re) {
		auto r = Relation::createUser();
		registerID(idSave, CharRE::create(TransLabel(r)));
		savedVariables.push_back({r, SavedVar(std::move(re), idRed == idSave ? NFA::ReductionType::Self :
						      NFA::ReductionType::Po, getRegisteredID(idRed))});
	}

	void registerViewID(std::string id, URE re) {
		auto r = Relation::createUser();
		registerID(std::move(id), CharRE::create(TransLabel(r)));
		savedVariables.push_back({r, SavedVar(std::move(re), VarStatus::View)});
	}

	void registerAssert(UCO c, const yy::location &loc) {
		asserts.push_back({std::move(c), DbgInfo(loc.end.filename, loc.end.line)});
	}

	// Handle "assume c" declaration in the input file
	void registerAssume(UCO c) {
		assumes.push_back(std::move(c));
	}

	void registerPPO(URE r) { ppo = std::move(r); }

	// Handle consistency constraint in the input file
	void addConstraint(UCO c, const std::string &s, const yy::location &loc);

	URE getRegisteredID(const std::string &id) const {
		auto it = variables.find(id);
		return (it == variables.end()) ? nullptr : it->second->clone();
	}

	bool isSavedID(const CharRE *re) const {
		auto ro = re->getLabel().getRelation();
		assert(ro.has_value());
		return std::find_if(savedVariables.begin(), savedVariables.end(),
				    [&](auto &p){ return p.first == *ro; }) != savedVariables.end();
	}

	URE getSavedID(const CharRE *re) const {
		assert(isSavedID(re));
		auto ro = re->getLabel().getRelation();
		return std::find_if(savedVariables.begin(), savedVariables.end(),
				    [&](auto &p){ return p.first == *ro; })->second.exp->clone();
	}

	const std::string &getRelationName(const Relation &r) const {
		auto rIt = std::find_if(variables.begin(), variables.end(), [&](auto &nre){
			auto *cRE = dynamic_cast<const CharRE *>(&*nre.second);
			return cRE && *cRE->getLabel().getRelation() == r;
		});
		assert(rIt != variables.end());
		return rIt->first;
	}

private:

	// Results from parsing the input file
	VarMap variables;

	// XXX: Maybe it's better to even have two containers below
	//      so that saved/reduced variables are treated differently,
	//      but I've no idea how many variable categories we want to have.
	//      If just two, I prefer separated. If more, polymorphism.
	SavedVarMap savedVariables;

	std::vector<Assert> asserts;
	std::vector<UCO> assumes;

	std::vector<URE>            acyclicityConstraints;
	std::vector<URE>            recoveryConstraints;
	std::vector<Inclusion<URE>> inclusionConstraints;
	URE ppo = nullptr;
};

#endif /* KATER_KAT_MODULE_HPP */
