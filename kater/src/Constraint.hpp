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

#ifndef KATER_CONSTRAINT_HPP
#define KATER_CONSTRAINT_HPP

#include "RegExp.hpp"
#include "Counterexample.hpp"
#include "Visitor.hpp"

/*******************************************************************************
 **                           Constraint Class (Abstract)
 ******************************************************************************/

class Constraint : public VisitableContainer {

public:
	enum class Type { Consistency, Error, Warning };

	using ValidFunT = std::function<bool(const TransLabel &)>;

protected:
	Constraint(std::vector<std::unique_ptr<RegExp> > &&kids = {})
		: kids(std::move(kids)) {}

public:
	virtual ~Constraint() = default;

	static std::unique_ptr<Constraint> createEmpty(std::unique_ptr<RegExp> e);

	static std::unique_ptr<Constraint>
	createEquality(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2);

	bool isEmpty() const;

	/* Returns the constraint type */
	Type getType() const { return type; }

	/* Sets the constraint type to T */
	void setType(Type t) { type = t; }

	/* Fetches the i-th kid */
	const RegExp *getKid(unsigned i) const {
		assert(i < kids.size() && "Index out of bounds!");
		return kids[i].get();
	}
	std::unique_ptr<RegExp> &getKid(unsigned i) {
		assert(i < kids.size() && "Index out of bounds!");
		return kids[i];
	}

	/* Sets the i-th kid to e */
	void setKid(unsigned i, std::unique_ptr<RegExp> e) {
		assert(i < getNumKids() && "Index out of bounds!");
		kids[i] = std::move(e);
	}

	/* Returns whether this RE has kids */
	bool hasKids() const { return !kids.empty(); }

	/* The kids of this RE */
	size_t getNumKids() const { return kids.size(); }

	/* Returns a clone of the Constraint */
	virtual std::unique_ptr<Constraint> clone() const = 0;

	/* Dumps the Constraint */
	virtual std::ostream &dump(std::ostream &s) const = 0;

protected:
	using KidsC = std::vector<std::unique_ptr<RegExp>>;

	const KidsC &getKids() const {
		return kids;
	}

	void addKid(std::unique_ptr<RegExp> k) {
		kids.push_back(std::move(k));
	}

	bool isContainer() const override { return false; }

	void visitChildren(BaseVisitor &visitor) const override {}

private:
	Type type = Type::Consistency;
	KidsC kids;
};

inline std::ostream &operator<<(std::ostream &s, Constraint::Type t)
{
	switch (t) {
	case Constraint::Type::Consistency:
		return s << "consistency";
	case Constraint::Type::Error:
		return s << "error";
	case Constraint::Type::Warning:
		return s << "warning";
	default:
		return s << "UKNOWN";
	}
	return s;
}

inline std::ostream &operator<<(std::ostream &s, const Constraint& re)
{
	s << "[" <<re.getType() << "] ";
	return re.dump(s);
}

/*******************************************************************************
 **                           Conjunctive Constraints
 ******************************************************************************/

class ConjunctiveConstraint : public Constraint {

protected:
	ConjunctiveConstraint(std::unique_ptr<Constraint> c1,
			      std::unique_ptr<Constraint> c2)
		: Constraint(), constraint1(std::move(c1)), constraint2(std::move(c2)) {  }

public:
	template<typename... Ts>
	static std::unique_ptr<ConjunctiveConstraint> create(Ts&&... params) {
		return std::unique_ptr<ConjunctiveConstraint>(
			new ConjunctiveConstraint(std::forward<Ts>(params)...));
	}

	std::unique_ptr<Constraint> clone() const override {
		return create(constraint1->clone(), constraint2->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << "(" << *constraint1 << " && " << *constraint2 << ")";
	}

	const Constraint *getConstraint1() const { return constraint1.get(); }
	const Constraint *getConstraint2() const { return constraint2.get(); }

private:
	bool isContainer() const override { return true; }

	void visitChildren(BaseVisitor &visitor) const override {
		visitor(*getConstraint1());
		visitor(*getConstraint2());
	}

	std::unique_ptr<Constraint> constraint1;
	std::unique_ptr<Constraint> constraint2;
};


/*******************************************************************************
 **                           Acyclicity Constraints
 ******************************************************************************/

class AcyclicConstraint : public Constraint {

protected:
	AcyclicConstraint(std::unique_ptr<RegExp> e) : Constraint() { addKid(std::move(e)); }

public:
	template<typename... Ts>
	static std::unique_ptr<AcyclicConstraint> create(Ts&&... params) {
		return std::unique_ptr<AcyclicConstraint>(
			new AcyclicConstraint(std::forward<Ts>(params)...));
	}

	/* NOTE: Might not return AcyclicConstraint */
	static std::unique_ptr<Constraint> createOpt(std::unique_ptr<RegExp> re);

	std::unique_ptr<Constraint> clone() const override { return create(getKid(0)->clone()); }

	std::ostream &dump(std::ostream &s) const override { return s << "acyclic" << *getKid(0); }
};


/*******************************************************************************
 **                           Recovery Constraints
 ******************************************************************************/

class RecoveryConstraint : public Constraint {

protected:
	RecoveryConstraint(std::unique_ptr<RegExp> e) : Constraint() { addKid(std::move(e)); }

public:
	template<typename... Ts>
	static std::unique_ptr<RecoveryConstraint> create(Ts&&... params) {
		return std::unique_ptr<RecoveryConstraint>(
			new RecoveryConstraint(std::forward<Ts>(params)...));
	}

	std::unique_ptr<Constraint> clone() const override { return create(getKid(0)->clone()); }

	std::ostream &dump(std::ostream &s) const override { return s << "recovery" << *getKid(0); }
};


/*******************************************************************************
 **                           Coherence Constraints
 ******************************************************************************/

class CoherenceConstraint : public Constraint {

protected:
	CoherenceConstraint(std::unique_ptr<RegExp> e) : Constraint() { addKid(std::move(e)); }

public:
	template<typename... Ts>
	static std::unique_ptr<CoherenceConstraint> create(Ts&&... params) {
		return std::unique_ptr<CoherenceConstraint>(
			new CoherenceConstraint(std::forward<Ts>(params)...));
	}

	/* NOTE: Might not return AcyclicConstraint */
	static std::unique_ptr<Constraint> createOpt(std::unique_ptr<RegExp> re);

	std::unique_ptr<Constraint> clone() const override { return create(getKid(0)->clone()); }

	std::ostream &dump(std::ostream &s) const override { return s << "coherence " << *getKid(0); }
};


/*******************************************************************************
 **                           Subset Constraints
 ******************************************************************************/

class SubsetConstraint : public Constraint {

protected:
	SubsetConstraint(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2)
		: Constraint() { addKid(std::move(e1)); addKid(std::move(e2)); }
public:
	template<typename... Ts>
	static std::unique_ptr<SubsetConstraint> create(Ts&&... params) {
		return std::unique_ptr<SubsetConstraint>(
			new SubsetConstraint(std::forward<Ts>(params)...));
	}

	/* NOTE: Might not return SubsetConstraint */
	static std::unique_ptr<Constraint>
	createOpt(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2);

	const RegExp *getLHS() const { return getKid(0); }
	RegExp *getLHS() { return getKid(0).get(); }

	const RegExp *getRHS() const { return getKid(1); }
	RegExp *getRHS() { return getKid(1).get(); }

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << *getKid(0) << " <= " << *getKid(1);
	}
};


/*******************************************************************************
 **                           SubsetSameEnds Constraint
 ******************************************************************************/

class SubsetSameEndsConstraint : public Constraint {

protected:
	SubsetSameEndsConstraint(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2)
		: Constraint() { addKid(std::move(e1)); addKid(std::move(e2)); }
public:
	template<typename... Ts>
	static std::unique_ptr<SubsetSameEndsConstraint> create(Ts&&... params) {
		return std::unique_ptr<SubsetSameEndsConstraint>(
			new SubsetSameEndsConstraint(std::forward<Ts>(params)...));
	}

	/* NOTE: Might not return SubsetSameEndsConstraint */
	static std::unique_ptr<Constraint>
	createOpt(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2);

	const RegExp *getLHS() const { return getKid(0); }
	RegExp *getLHS() { return getKid(0).get(); }

	const RegExp *getRHS() const { return getKid(1); }
	RegExp *getRHS() { return getKid(1).get(); }

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << *getKid(0) << " <=&id " << *getKid(1);
	}
};


/*******************************************************************************
 **                           Totality Constraint
 ******************************************************************************/

class TotalityConstraint : public Constraint {

protected:
	TotalityConstraint(std::unique_ptr<RegExp> e)
		: Constraint() { addKid(std::move(e)); }
public:
	template<typename... Ts>
	static std::unique_ptr<TotalityConstraint> create(Ts&&... params) {
		return std::unique_ptr<TotalityConstraint>(
			new TotalityConstraint(std::forward<Ts>(params)...));
	}

	static std::unique_ptr<Constraint>
	createOpt(std::unique_ptr<RegExp> e) {
		return create(std::move(e));
	}

	const RegExp *getRelation() const { return getKid(0); }
	RegExp *getRelation() { return getKid(0).get(); }

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << "total " << *getKid(0);
	}
};


/*******************************************************************************
 **                           Transitivity Constraint
 ******************************************************************************/

class TransitivityConstraint : public Constraint {

protected:
	TransitivityConstraint(std::unique_ptr<RegExp> e)
		: Constraint() { addKid(std::move(e)); }
public:
	template<typename... Ts>
	static std::unique_ptr<TransitivityConstraint> create(Ts&&... params) {
		return std::unique_ptr<TransitivityConstraint>(
			new TransitivityConstraint(std::forward<Ts>(params)...));
	}

	static std::unique_ptr<Constraint>
	createOpt(std::unique_ptr<RegExp> e) {
		return create(std::move(e));
	}

	const RegExp *getRelation() const { return getKid(0); }
	RegExp *getRelation() { return getKid(0).get(); }

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << "transitive " << *getKid(0);
	}
};

#endif /* KATER_CONSTRAINT_HPP */
