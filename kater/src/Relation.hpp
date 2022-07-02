#include "Predicate.hpp"

/*******************************************************************************
 **                           RelationInfo Class
 ******************************************************************************/

enum class RelType  { OneOne, ManyOne, UnsuppMany, Conj, Final };

struct RelationInfo {
	std::string  name;
	RelType      type;
	PredicateSet dom;
	PredicateSet codom;
	bool         insidePo;
};


/*******************************************************************************
 **                           Relation Class
 ******************************************************************************/

class Relation {

public:
	/* Negative IDs reserved for user-defined relations. */
	using ID = int;

        enum Builtin {
		/*** CAUTION: The dummy IDs should not be a part of the builtin map */

		/* same thread */
		same_thread,
		/* same location */
		alloc,
		frees,
		loc_overlap,
		/* tc, tj */
		tc,
		tj,
		/* rf, co, fr */
		PerLocBegin,
		rf,
		mo_imm,
		fr_imm,
		/* rfe, coe, fre */
		rfe,
		moe,
		fre,
		/* deps */
		WithinPoBegin,
		ctrl_imm,
		addr_imm,
		data_imm,
		/* rfi, coi, fri*/
		rfi,
		moi,
		fri,
		/* detour */
		detour,
		/* po-loc */
		po_loc_imm,
		WithinPoLast,
		PerLocLast,
		/* po */
		po_imm,
		/* any */
		any,
	};

	using builtin_iterator = std::unordered_map<Relation::Builtin,
						    RelationInfo>::iterator;
	using builtin_const_iterator = std::unordered_map<Relation::Builtin,
							  RelationInfo>::const_iterator;

protected:
	Relation() = delete;
	Relation(ID id, bool inverse = false) : id(id), inverse(inverse) {}

public:
	static Relation createBuiltin(Builtin b) { return Relation(getBuiltinID(b)); }
	static Relation createUser() { return Relation(getFreshID()); }

	static ID getBuiltinID(Builtin b) { return b; }

	static builtin_const_iterator builtin_begin() { return builtins.begin(); }
	static builtin_const_iterator builtin_end() { return builtins.end(); }

	constexpr ID getID() const { return id; }

	constexpr bool isBuiltin() const { return getID() >= 0; }

	constexpr Builtin toBuiltin() const {
		assert(isBuiltin());
		return static_cast<Builtin>(getID());
	}

	void markAsPerLocOf(const Relation &other) const {
		perlocs.insert({other.getID(), *this});
	}

	Relation getPerLoc() const {
		assert(perlocs.count(this->getID()));
		return perlocs.find(this->getID())->second;
	}

	bool hasPerLoc() const { return perlocs.count(this->getID()); }

	/* Inverses this relation */
	void invert() { inverse = !inverse; }

	/* Whether this relation is inversed */
	constexpr bool isInverse() const { return inverse; }

	/* ***builtins only*** returns the domain of the relation */
	const PredicateSet &getDomain() const;

	/* ***builtins only*** returns the codomain of the relation */
	const PredicateSet &getCodomain() const;

	std::string getName() const;

	bool operator==(const Relation &other) const {
		return getID() == other.getID() && isInverse() == other.isInverse();
	}
	bool operator!=(const Relation &other) const {
		return !(*this == other);
	}

	bool operator<(const Relation &other) const {
		return getID() < other.getID() ||
		       (getID() == other.getID() && isInverse() < other.isInverse());
	}
	bool operator>=(const Relation &other) const {
		return !(*this < other);
	}

	bool operator>(const Relation &other) const {
		return getID() > other.getID() ||
		       (getID() == other.getID() && isInverse() > other.isInverse());
	}
	bool operator<=(const Relation &other) const {
		return !(*this > other);
	}

private:
	static ID getFreshID() { return --dispenser; }

	ID id;
	bool inverse = false;

	static inline ID dispenser = 0;
	static const std::unordered_map<Relation::Builtin, RelationInfo> builtins;
	static std::unordered_map<Relation::ID, Relation> perlocs;
};

struct RelationHasher {
    size_t operator()(const Relation &r) const {
	    return std::hash<Relation::ID>()(r.getID());
    }
};
