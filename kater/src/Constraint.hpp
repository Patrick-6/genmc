#ifndef __CONSTRAINT_HPP__
#define __CONSTRAINT_HPP__

#include "RegExp.hpp"

/*******************************************************************************
 **                           Constraint Class (Abstract)
 ******************************************************************************/

class Constraint {

protected:
	Constraint(std::vector<std::unique_ptr<RegExp> > &&kids = {})
		: kids(std::move(kids)) {}

public:
	virtual ~Constraint() = default;

	/* Fetches the i-th kid */
	const RegExp *getKid(unsigned i) const {
		assert(i < kids.size() && "Index out of bounds!");
		return kids[i].get();
	}
	RegExp *getKid(unsigned i) {
		return const_cast<RegExp *>(static_cast<const Constraint &>(*this).getKid(i));
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

private:
	KidsC kids;
};

inline std::ostream &operator<<(std::ostream &s, const Constraint& re)
{
	return re.dump(s);
}


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
	RegExp *getLHS() { return getKid(0); }

	const RegExp *getRHS() const { return getKid(1); }
	RegExp *getRHS() { return getKid(1); }

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << *getKid(0) << " <= " << *getKid(1);
	}
};


/*******************************************************************************
 **                           Empty Constraints
 ******************************************************************************/

class EmptyConstraint : public Constraint {

protected:
	EmptyConstraint(std::unique_ptr<RegExp> e) : Constraint() {
		addKid(std::move(e));
	}
public:
	template<typename... Ts>
	static std::unique_ptr<EmptyConstraint> create(Ts&&... params) {
		return std::unique_ptr<EmptyConstraint>(
			new EmptyConstraint(std::forward<Ts>(params)...));
	}

	/* NOTE: Might not return EmptyConstraint */
	static std::unique_ptr<Constraint> createOpt(std::unique_ptr<RegExp> e);

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << *getKid(0) << " = 0";
	}
};

#endif /* __CONSTRAINT_HPP__ */
