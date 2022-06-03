#ifndef __CONSTRAINT_HPP__
#define __CONSTRAINT_HPP__

#include "RegExp.hpp"

/*******************************************************************************
 **                           Constraint Class (Abstract)
 ******************************************************************************/

class Constraint {

public:
	enum class Type { Consistency, Error, Warning };

	using ValidFunT = std::function<bool(const TransLabel &)>;

protected:
	Constraint(std::vector<std::unique_ptr<RegExp> > &&kids = {})
		: kids(std::move(kids)) {}

public:
	virtual ~Constraint() = default;

	static std::unique_ptr<Constraint> createEmpty(std::unique_ptr<RegExp> e);

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

	/* Does the Constraint hold statically */
	virtual bool checkStatically(const std::vector<std::unique_ptr<Constraint>> &assm,
				     std::string &cex,
				     ValidFunT vfun = [](auto &t){ return true; }) const {
		return false;
	}

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

	bool checkStatically(const std::vector<std::unique_ptr<Constraint>> &assm,
			     std::string &cex,
			     ValidFunT vfun = [](auto &t){ return true; }) const override;

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << *getKid(0) << " <= " << *getKid(1);
	}
};


/*******************************************************************************
 **                           Equality Constraints
 ******************************************************************************/

class EqualityConstraint : public Constraint {

protected:
	EqualityConstraint(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2)
		: Constraint() { addKid(std::move(e1)); addKid(std::move(e2)); }
public:
	template<typename... Ts>
	static std::unique_ptr<EqualityConstraint> create(Ts&&... params) {
		return std::unique_ptr<EqualityConstraint>(
			new EqualityConstraint(std::forward<Ts>(params)...));
	}

	/* NOTE: Might not return EqualityConstraint */
	static std::unique_ptr<Constraint>
	createOpt(std::unique_ptr<RegExp> e1, std::unique_ptr<RegExp> e2);

	const RegExp *getLHS() const { return getKid(0); }
	RegExp *getLHS() { return getKid(0).get(); }

	const RegExp *getRHS() const { return getKid(1); }
	RegExp *getRHS() { return getKid(1).get(); }

	bool checkStatically(const std::vector<std::unique_ptr<Constraint>> &assm,
			     std::string &cex,
			     ValidFunT vfun = [](auto &t){ return true; }) const override;

	std::unique_ptr<Constraint> clone() const override {
		return create(getKid(0)->clone(), getKid(1)->clone());
	}

	std::ostream &dump(std::ostream &s) const override {
		return s << *getKid(0) << " = " << *getKid(1);
	}
};

#endif /* __CONSTRAINT_HPP__ */
