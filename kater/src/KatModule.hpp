#ifndef __KAT_MODULE_HPP__
#define __KAT_MODULE_HPP__

#include "Inclusion.hpp"
#include "KatModuleAPI.hpp"
#include "Parser.hpp"

class KatModule {

public:
	using UCO = std::unique_ptr<Constraint>;

private:
	using VarMap = std::unordered_map<std::string, URE>;
	using SavedVarSet = std::vector<SavedVar>;

public:

	KatModule() = default;

	using var_iter = VarMap::iterator;
	using var_const_iter = VarMap::const_iterator;
	using svar_iter = SavedVarSet::iterator;
	using svar_const_iter = SavedVarSet::const_iterator;
	using acyc_iter = std::vector<URE>::iterator;
	using acyc_const_iter = std::vector<URE>::const_iterator;
	using incl_iter = std::vector<Inclusion<URE>>::iterator;
	using incl_const_iter = std::vector<Inclusion<URE>>::const_iterator;
	using assr_iter = std::vector<std::pair<UCO, yy::location>>::iterator;
	using assr_const_iter = std::vector<std::pair<UCO, yy::location>>::const_iterator;

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

	incl_iter incl_begin() { return inclusionConstraints.begin(); }
	incl_iter incl_end() { return inclusionConstraints.end(); }
	incl_const_iter incl_begin() const { return inclusionConstraints.begin(); }
	incl_const_iter incl_end() const { return inclusionConstraints.end(); }

	assr_iter assert_begin() { return asserts.begin(); }
	assr_iter assert_end() { return asserts.end(); }
	assr_const_iter assert_begin() const { return asserts.begin(); }
	assr_const_iter assert_end() const { return asserts.end(); }

	size_t getRegisteredNum() const { return variables.size(); }

	size_t getSavedNum() const { return savedVariables.size(); }

	size_t getAssertNum() const { return asserts.size(); }

	size_t getAcyclicNum() const { return acyclicityConstraints.size(); }

	size_t getInclusionNum() const { return inclusionConstraints.size(); }

	void registerID(std::string id, URE re) {
		variables.insert({id, std::move(re)});
	}

	void registerSaveID(std::string id, URE re) {
		registerID(std::move(id), RelRE::create(RelLabel::getFreshCalcLabel()));
		savedVariables.push_back({std::move(re)});
	}

	void registerSaveReducedID(std::string idSave, std::string idRed, URE re) {
		registerID(idSave, RelRE::create(RelLabel::getFreshCalcLabel()));
		savedVariables.push_back({std::move(re), idRed == idSave ? NFA::ReductionType::Self :
					  NFA::ReductionType::Po, getRegisteredID(idRed)});
	}

	void registerViewID(std::string id, URE re) {
		registerID(std::move(id), RelRE::create(RelLabel::getFreshCalcLabel()));
		savedVariables.push_back({std::move(re), VarStatus::View});
	}

	void registerAssert(UCO c, const yy::location &loc) {
		asserts.push_back({std::move(c), loc});
	}

	// Handle "assume c" declaration in the input file
	void registerAssume(UCO c, const yy::location &loc);

	// Handle consistency constraint in the input file
	void addConstraint(UCO c, const std::string &s, const yy::location &loc);

	URE getRegisteredID(const std::string &id) {
		auto it = variables.find(id);
		return (it == variables.end()) ? nullptr : it->second->clone();
	}

	URE getSavedID(const RelRE *re) const {
		return savedVariables[re->getLabel().getCalcIndex()].exp->clone();
	}

private:

	// Results from parsing the input file
	VarMap variables;

	// XXX: Maybe it's better to even have two containers below
	//      so that saved/reduced variables are treated differently,
	//      but I've no idea how many variable categories we want to have.
	//      If just two, I prefer separated. If more, polymorphism.
	SavedVarSet savedVariables;

	std::vector<std::pair<UCO, yy::location>> asserts;

	std::vector<URE>            acyclicityConstraints;
	std::vector<Inclusion<URE>> inclusionConstraints;
};

#endif /* __KAT_MODULE_HPP__ */
