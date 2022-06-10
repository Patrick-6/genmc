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
		/* po */
		po_imm = 0,
		po_loc_imm,
		/* deps */
		ctrl_imm,
		addr_imm,
		data_imm,
		/* same thread */
		same_thread,
		/* same location */
		alloc,
		frees,
		loc_overlap,
		/* rf, co, fr, detour */
		rf,
		rfe,
		rfi,
		tc,
		tj,
		mo_imm,
		moe,
		moi,
		fr_imm,
		fre,
		fri,
		detour,
	};

	using builtin_iterator = std::unordered_map<Relation::Builtin,
						    RelationInfo>::iterator;
	using builtin_const_iterator = std::unordered_map<Relation::Builtin,
							  RelationInfo>::const_iterator;

protected:
	Relation() = delete;
	Relation(ID id) : id(id) {}

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

	/* ***builtins only*** returns the domain of the relation */
	const PredicateSet &getDomain() const;

	/* ***builtins only*** returns the codomain of the relation */
	const PredicateSet &getCodomain() const;

	std::string getName() const;

	bool operator==(const Relation &other) const {
		return getID() == other.getID();
	}
	bool operator!=(const Relation &other) const {
		return !(*this == other);
	}

	bool operator<(const Relation &other) const {
		return getID() < other.getID();
	}
	bool operator>=(const Relation &other) const {
		return !(*this < other);
	}

	bool operator>(const Relation &other) const {
		return getID() > other.getID();
	}
	bool operator<=(const Relation &other) const {
		return !(*this > other);
	}

private:
	static ID getFreshID() { return --dispenser; }

	ID id;

	static inline ID dispenser = 0;
	static const std::unordered_map<Relation::Builtin, RelationInfo> builtins;
};

struct RelationHasher {
    size_t operator()(const Relation &r) const {
	    return std::hash<Relation::ID>()(r.getID());
    }
};
