#ifndef _KATER_NFA_HPP_
#define _KATER_NFA_HPP_

#include "TransLabel.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class NFA {

public:
	enum class ReductionType { Po, Self };

public:
	class State;

	/* There are no heterogenous lookups before C++20,
	 * so let's just use a UPs with a custom deleter  */
	template<class T>
	struct maybe_deleter{
		bool _delete;
		explicit maybe_deleter(bool doit = true) : _delete(doit) {}

		void operator()(T* p) const{
			if(_delete) delete p;
		}
	};

	template<class T>
	using set_unique_ptr = std::unique_ptr<T, maybe_deleter<T>>;

	template<class T>
	set_unique_ptr<T> make_find_ptr(T* raw){
		return set_unique_ptr<T>(raw, maybe_deleter<T>(false));
	}

	using StateUPVectorT = std::vector<set_unique_ptr<State>>;
	using StateVectorT = std::vector<State *>;

	/*
	 * A struct representing NFA transitions
	 */
	struct Transition {
		TransLabel label;
		State *dest;

		Transition() = delete;
		Transition(const TransLabel &lab, State *dest)
			: label(lab), dest(dest) {}

		Transition copyTo(State *s) const {
			auto l = label;
			return Transition(l, s);
		}

		/* Returns a transition with the label flipped,
		 * and the destination changed to DEST */
		Transition flipTo(State *s) const {
			auto l = label;
			l.flip();
			return Transition(l, s);
		}

		inline bool operator==(const Transition &t) const {
			return dest == t.dest && label == t.label;
		}
		inline bool operator!=(const Transition &t) const {
			return !(*this == t);
		}

		inline bool operator<(const Transition &t) const {
			return (dest < t.dest) || (dest == t.dest && label < t.label);
		}
		inline bool operator>=(const Transition &t) const {
			return !(*this < t);
		}
		inline bool operator>(const Transition &t) const {
			return (*this >= t) && (*this != t);
		}
		inline bool operator<=(const Transition &t) const {
			return !(*this > t);
		}
	};


	/*
	 * A class representing NFA states.
	 * Each state maintains a collections of its transitions.
	 */
	class State {

	private:
		/* FIXME: Do not use set */
		using TransitionsC = std::set<Transition>;

	public:
		State() = delete;
		State(unsigned id) : id(id), starting(false), accepting(false),
				     transitions(), inverse() {}

		using trans_iterator = TransitionsC::iterator;
		using trans_const_iterator = TransitionsC::const_iterator;

		trans_iterator out_begin() { return getOutgoing().begin(); }
		trans_iterator out_end() { return getOutgoing().end(); }

		trans_const_iterator out_begin() const { return getOutgoing().begin(); }
		trans_const_iterator out_end() const { return getOutgoing().end(); }

		trans_iterator in_begin() { return getIncoming().begin(); }
		trans_iterator in_end() { return getIncoming().end(); }

		trans_const_iterator in_begin() const { return getIncoming().begin(); }
		trans_const_iterator in_end() const { return getIncoming().end(); }

		/* Returns this state's ID */
		unsigned getId() const { return id; }

		/* Whether this state is a starting state of the NFA */
		bool isStarting() const { return starting; }

		/* Whether this state is an accepting state of the NFA */
		bool isAccepting() const { return accepting; }

		void setStarting(bool b) { starting = b; }
		void setAccepting(bool b) { accepting = b; }

		/* Whether this state has an outgoing transition T */
		bool hasOutgoing(const Transition &t) const {
			return getOutgoing().count(t);
		}

		/* Whether this state has any outgoing transitions */
		bool hasOutgoing() const { return out_begin() != out_end(); }

		size_t getNumOutgoing() const { return getOutgoing().size(); }

		/* Whether this state has an inverse transition T */
		bool hasIncoming(const Transition &t) const {
			return getIncoming().count(t);
		}

		/* Whether this state has any incoming transitions */
		bool hasIncoming() const { return in_begin() != in_end(); }

		size_t getNumIncoming() const { return getIncoming().size(); }

		bool hasOutgoingTo(State *s) const {
			return std::any_of(out_begin(), out_end(), [&](const Transition &t){
					return t.dest == s;
				});
		}

		bool hasIncomingTo(State *s) const {
			return std::any_of(in_begin(), in_end(), [&](const Transition &t){
					return t.dest == s;
				});
		}

		/* Adds an outgoing transition T (if it doesn't exist) */
		void addOutgoing(const Transition &t) {
			getOutgoing().insert(t);
		}

		/* Adds an incoming transition T (if it doesn't exist) */
		void addIncoming(const Transition &t) {
			getIncoming().insert(t);
		}

		/* Removes outgoing transition T from this state (if exists) */
		void removeOutgoing(const Transition &t) {
			getOutgoing().erase(t);
		}

		/* Removes incoming transition T from this state (if exists) */
		void removeIncoming(const Transition &t) {
			getIncoming().erase(t);
		}

		/* Removes all outgoing with S as their dest */
		void removeOutgoingTo(State *s) {
			for (auto it = out_begin(), ie = out_end(); it != ie; /* */) {
				if (it->dest == s)
					it = getOutgoing().erase(it);
				else
					++it;
			}
		}

		/* Removes all incoming with S as their dest */
		void removeIncomingTo(State *s) {
			for (auto it = in_begin(), ie = in_end(); it != ie; /* */) {
				if (it->dest == s)
					it = getIncoming().erase(it);
				else
					++it;
			}
		}

		/* Returns true if all outgoing transitions of this
		 * state return to this state */
		bool hasAllOutLoops() const {
			return std::all_of(out_begin(), out_end(), [&](const Transition &t){
				return t.dest == this;
			});
		}

		/* Returns true if all incoming transition of this
		 * state return to this state */
		bool hasAllInLoops() const {
			return std::all_of(in_begin(), in_end(), [&](const Transition &t){
				return t.dest == this;
			});
		}

		bool hasAllOutPredicates() const {
			return std::all_of(out_begin(), out_end(), [](auto &t){
				return t.label.isPredicate();
			});
		}

		bool hasAllInPredicates() const {
			return std::all_of(in_begin(), in_end(), [](auto &t){
				return t.label.isPredicate();
			});
		}

		/* Whether all outgoing transitions are the same as the ones of S */
		bool outgoingSameAs(State *s) const {
			return getOutgoing() == s->getOutgoing();
		}

		/* Whether all incoming transitions are the same as the ones of S */
		bool incomingSameAs(State *s) const {
			return getIncoming() == s->getIncoming();
		}

		State &flip() {
			std::swap(getOutgoing(), getIncoming());
			std::swap(starting, accepting);
			return *this;
		}

	private:
		const TransitionsC &getOutgoing() const { return transitions; }
		TransitionsC &getOutgoing() { return transitions; }

		const TransitionsC &getIncoming() const { return inverse; }
		TransitionsC &getIncoming() { return inverse; }

		unsigned id;
		bool starting;
		bool accepting;
		TransitionsC transitions;
		TransitionsC inverse;
	};

public:
	NFA() = default;
	NFA(const TransLabel &label);

	using stateUP_iterator = StateUPVectorT::iterator;
	using stateUP_const_iterator = StateUPVectorT::const_iterator;

	stateUP_iterator states_begin() { return getStates().begin(); }
	stateUP_iterator states_end() { return getStates().end(); }

	stateUP_const_iterator states_begin() const { return getStates().begin(); }
	stateUP_const_iterator states_end() const { return getStates().end(); }

	using state_iterator = StateVectorT::iterator;
	using state_const_iterator = StateVectorT::const_iterator;

	state_iterator start_begin() { return getStarting().begin(); }
	state_iterator start_end() { return getStarting().end(); }

	state_const_iterator start_begin() const { return getStarting().begin(); }
	state_const_iterator start_end() const { return getStarting().end(); }

	state_iterator accept_begin() { return getAccepting().begin(); }
	state_iterator accept_end() { return getAccepting().end(); }

	state_const_iterator accept_begin() const { return getAccepting().begin(); }
	state_const_iterator accept_end() const { return getAccepting().end(); }

	unsigned getNumStates() const { return getStates().size(); }

	unsigned getNumStarting() const { return getStarting().size(); }

	unsigned getNumAccepting() const { return getAccepting().size(); }

	bool isStarting(State *state) const { return state->isStarting(); }

	bool isAccepting(State *state) const { return state->isAccepting(); }

	/* Creates and adds a new (unreachable) state to the NFA and its inverse.
	 * Returns the newly added state */
	State *createState() {
		static unsigned counter = 0;
		return getStates().emplace_back(new State(counter++)).get();
	}

	State *createStarting() {
		auto *s = createState();
		s->setStarting(true);
		getStarting().push_back(s);
		return s;
	}

	State *createAccepting() {
		auto *s = createState();
		s->setAccepting(true);
		getAccepting().push_back(s);
		return s;
	}

	void makeStarting(State *s) {
		if (!isStarting(s)) {
			s->setStarting(true);
			getStarting().push_back(s);
		}
	}

	void makeAccepting(State *s) {
		if (!isAccepting(s)) {
			s->setAccepting(true);
			getAccepting().push_back(s);
		}
	}

	void clearStarting() {
		std::for_each(start_begin(), start_end(), [&](auto &s){
			s->setStarting(false);
		});
		getStarting().clear();
	}

	void clearAccepting() {
		std::for_each(accept_begin(), accept_end(), [&](auto &s){
			s->setAccepting(false);
		});
		getAccepting().clear();
	}

	stateUP_iterator removeState(stateUP_iterator &it) {
		std::for_each(states_begin(), states_end(), [&](decltype(*states_begin()) &s){
			s->removeOutgoingTo(it->get());
			s->removeIncomingTo(it->get());
		});
		auto sit = std::find(getStarting().begin(), getStarting().end(), it->get());
		if (sit != getStarting().end())
			getStarting().erase(sit);
		auto ait = std::find(getAccepting().begin(), getAccepting().end(), it->get());
		if (ait != getAccepting().end())
			getAccepting().erase(ait);
		return getStates().erase(it);
	}

	/* Removes STATE from the NFA and its inverse */
	void removeState(State *state) {
		auto it = std::find(states_begin(), states_end(), make_find_ptr(state));
		if (it != states_end())
			removeState(it);
	}

	template<typename ITER>
	void removeStates(ITER &&begin, ITER &&end) {
		std::for_each(begin, end, [&](State *s){ removeState(s); });
	}

	/* Whether the (regular) NFA has a transition T from state SRC */
	bool hasTransition(State *src, const Transition &t) const {
		return src->hasOutgoing(t);
	}

	/* Adds a transition to the NFA and updates the inverse */
	void addTransition(State *src, const Transition &t) {
		src->addOutgoing(t);
		t.dest->addIncoming(t.flipTo(src));
	}

	template<typename ITER>
	void addTransitions(State *src, ITER &&begin, ITER &&end) {
		std::for_each(begin, end, [&](const Transition &t){
			addTransition(src, t);
		});
	}

	template<typename ITER>
	void addInvertedTransitions(State *dst, ITER &&begin, ITER &&end) {
		std::for_each(begin, end, [&](const Transition &t){
			addTransition(t.dest, t.flipTo(dst));
		});
	}

	void addSelfTransition(State *src, const TransLabel &lab) {
		addTransition(src, Transition(lab, src));
	}

	/*
	 * Adds an ε transition from SRC to DST by connecting SRC to
	 * all of DST's successors (using the respective labels).
	 */
	void addEpsilonTransitionSucc(State *src, State *dst) {
		addTransitions(src, dst->out_begin(), dst->out_end());
		if (isAccepting(dst))
			makeAccepting(src);
		if (isStarting(src))
			makeStarting(dst);
	}

	/*
	 * Adds an ε transition from SRC to DST by connecting all
	 * of SRC's predecessors to DST (using the respective labels).
	 */
	void addEpsilonTransitionPred(State *src, State *dst) {
		addInvertedTransitions(dst, src->in_begin(), src->in_end());
		if (isAccepting(dst))
			makeAccepting(src);
		if (isStarting(src))
			makeStarting(dst);
	}

	State *addTransitionToFresh(State *src, const TransLabel &lab) {
		auto *dst = createState();
		addTransition(src, Transition(lab, dst));
		return dst;
	}

	/* Removes a transition T from S and updates the inverse */
	void removeTransition(State *src, const Transition &t) {
		src->removeOutgoing(t);
		t.dest->removeIncoming(t.flipTo(src));
	}

	template<typename ITER>
	void removeTransitions(State *src, ITER &&begin, ITER &&end){
		std::for_each(begin, end, [&](const Transition &t){
			removeTransition(src, t);
		});
	}

	/* Remove all of SRC's transitions that satisfy PRED */
	template<typename F>
	void removeTransitionsIf(State *src, F&& pred) {
		std::vector<Transition> toRemove;
		std::copy_if(src->out_begin(), src->out_end(), std::back_inserter(toRemove),
			     [&](const Transition &t){ return pred(t); });
		removeTransitions(src, toRemove.begin(), toRemove.end());
	}

	void removeAllTransitions(State *src) {
		removeTransitionsIf(src, [&](const Transition &t){ return true; });
	}

	NFA &flip();
	NFA &alt(NFA &&other);
	NFA &seq(NFA &&other);
	NFA &or_empty();
	NFA &star();
	NFA &plus();

	NFA &simplify(std::function<bool(const TransLabel &)> isValidTransition =
		      [](const TransLabel &lab){ return true; });
	NFA &reduce(ReductionType t);

	NFA &removeDeadStates();

	NFA &composePredicateEdges();

	NFA &breakToParts();

	bool acceptsEmptyString() const;
	bool acceptsNoString(std::string &cex) const;
	bool isSubLanguageOfDFA(const NFA &other, std::string &cex) const;

	std::pair<NFA, std::map<State *, std::set<State *>>> to_DFA () const;

	NFA copy(std::unordered_map<State *, State *> *mapping = nullptr) const;

	friend std::ostream& operator<< (std::ostream& ostr, const NFA& nfa);
	template<typename T>
	friend std::ostream & operator<< (std::ostream& ostr, const std::set<T> &s);
	template<typename ITER>
	friend std::unordered_map<NFA::State *, unsigned> assignStateIDs(ITER &&begin, ITER &&end);

private:
	template<typename F>
	void applyBidirectionally(F &&fun) {
		fun();
		flip();
		fun();
		flip();
	}

	void breakIntoMultiple(State *s, const Transition &t);

	void simplify_basic();

	bool joinPredicateEdges(std::function<bool(const TransLabel &)> isValidTransition);

	void removeRedundantSelfLoops();

	void compactEdges(std::function<bool(const TransLabel &)> isValidTransition);

	void removeDeadStatesDFS();

	std::unordered_set<State *> calculateUsefulStates();

	void scm_reduce ();
	std::unordered_map<State *, std::vector<char>> get_state_composition_matrix ();

	const StateUPVectorT &getStates() const { return nfa; }
	StateUPVectorT &getStates() { return nfa; }

	const StateVectorT &getAccepting() const { return accepting; }
	StateVectorT &getAccepting() { return accepting; }

	const StateVectorT &getStarting() const { return starting; }
	StateVectorT &getStarting() { return starting; }

	StateUPVectorT nfa;
	StateVectorT starting;
	StateVectorT accepting;
};

#endif /* _KATER_NFA_HPP_ */
