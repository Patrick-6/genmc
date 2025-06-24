/*
 * GenMC -- Generic Model Checking.
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
 *
 * Author: Michalis Kokologiannakis <michalis@mpi-sws.org>
 */

#ifndef GENMC_SCHEDULER_HPP
#define GENMC_SCHEDULER_HPP

#include "ADT/Trie.hpp"
#include "ADT/View.hpp"
#include "Config/Config.hpp"
#include "ExecutionGraph/ExecutionGraph.hpp"
#include "Runtime/InterpreterEnumAPI.hpp"

#include <optional>
#include <random>
#include <vector>

class GenMCDriver;

class Scheduler {
public:
	Scheduler() = delete;
	Scheduler(const Scheduler &) = delete;
	Scheduler(Scheduler &&) = default;
	auto operator=(const Scheduler &) -> Scheduler & = delete;
	auto operator=(Scheduler &&) -> Scheduler & = default;
	virtual ~Scheduler() = default;

	static auto create(GenMCDriver *driver) -> std::unique_ptr<Scheduler>;

	/** This function should be called at the beginning of every execution. */
	void resetExplorationOptions(const ExecutionGraph &g);

	/**
	 * Returns the next thread to run the interpreter on.
	 * The selected thread will be any unblocked prioritized thread if available, otherwise a
	 * thread is selected according to the current scheduling policy. Finally, if all threads
	 * are blocked or finished and any thread has a read to reschedule, it will be unblocked to
	 * run next.
	 * This function should only be used after the `TryOptimizeScheduling` phase is
	 * completed.
	 *   */
	[[nodiscard]] auto scheduleNext(ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int>;

	/** Opt: Whether the exploration should try to repair R */
	[[nodiscard]] auto isRescheduledRead(Event r) const -> bool
	{
		return readToReschedule_ == r;
	}

	void cacheEventLabel(const ExecutionGraph &g, const EventLabel *lab);

	/** Opt: Sets R as a read to be repaired */
	void setRescheduledRead(Event r) { readToReschedule_ = r; }

	/** Set a thread to be prioritised */
	void prioritize(Event pos) { threadPrios_ = {pos}; }

	/** Add another thread to be prioritised */
	void addThreadPrio(Event pos) { threadPrios_.push_back(pos); }

	/** Resets the prioritization scheme */
	void resetThreadPrioritization() { threadPrios_.clear(); }

	/** Pre-calculate the replay schedule based on the `ExecutionGraph` and set the scheduling
	 * phase to replay. This function should only be called once per execution, once no more
	 * events can be added from the cache in `Phase::TryOptimizeScheduling`. */
	void enterReplayPhase(const ExecutionGraph &g);

protected:
	Scheduler(GenMCDriver *driver) : driver_(driver) {}

	[[nodiscard]] auto getDriver() const -> const GenMCDriver * { return driver_; }
	auto getDriver() -> GenMCDriver * { return driver_; }

private:
	enum class Phase : std::uint8_t {
		TryOptimizeScheduling,
		Replay,
		Exploration,
	};

	using ValuePrefixT = std::unordered_map<
		std::pair<unsigned int, unsigned int>, // fun_id, tid
		Trie<std::vector<SVal>, std::vector<std::unique_ptr<EventLabel>>, SValUCmp>,
		PairHasher<unsigned int, unsigned int>>;

	[[nodiscard]] auto getPhase() const -> Phase { return phase_; }

	/** Opt: Checks whether SEQ has been seen before for <FUN_ID, TID> and
	 * if so returns its successors. Returns nullptr otherwise. */
	auto retrieveCachedSuccessors(std::pair<unsigned int, unsigned int> key,
				      const std::vector<SVal> &seq)
		-> std::vector<std::unique_ptr<EventLabel>> *
	{
		return seenPrefixes[key].lookup(seq);
	}

	/** Opt: Try to extend a thread in the ExecutionGraph with events from the cache.
	 *  Returns `false` if the de-caching failed. */
	auto tryOptimizeScheduling(const ExecutionGraph &g) -> bool;

	/** Opt: Try to extend a specific thread in the ExecutionGraph with events from the cache.
	 *  Returns `false` if the de-caching failed. */
	auto scheduleFromCache(const ExecutionGraph &g, int thread) -> std::optional<Event>;

	/** Tries to schedule according to the current prioritization scheme */
	[[nodiscard]] auto schedulePrioritized(const ExecutionGraph &g) -> std::optional<int>;

	/** Opt: Tries to reschedule any reads that were added blocked */
	[[nodiscard]] auto rescheduleReads(ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int>;

	auto getNextThreadToReplay(const ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int>;

	/** Schedules a thread according to a policy */
	[[nodiscard]] virtual auto scheduleWithPolicy(const ExecutionGraph &g,
						      std::span<Action> runnable)
		-> std::optional<int> = 0;

	GenMCDriver *driver_{};

	Scheduler::Phase phase_{};

	/**
	 * Pre-calculated replay schedule containing positions that should be replayed in order.
	 * The thread of the event at the `back` of `replaySchedule_` is always the one to be
	 * replayed next, until the event is in the execution graph. Once the event is in the graph,
	 * it can be popped from the replay schedule. Depending on the interpreter, this may
	 * correspond to multiple calls to `scheduleNext`.
	 */
	std::vector<Event> replaySchedule_;

	/** Opt: Whether a particular read needs to be repaired during rescheduling */
	Event readToReschedule_ = Event::getInit();

	/** Opt: Which thread(s) the scheduler should prioritize
	 * (empty if none) */
	std::vector<Event> threadPrios_;

	/** Opt: Cached labels for optimized scheduling */
	ValuePrefixT seenPrefixes;
};

class ArbitraryScheduler : public Scheduler {
public:
	ArbitraryScheduler(GenMCDriver *driver);

protected:
	using MyRNG = std::mt19937;
	using MyDist = std::uniform_int_distribution<MyRNG::result_type>;

	auto getRng() -> MyRNG & { return rng_; }

private:
	auto scheduleWithPolicy(const ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int> override;

	/** Dbg: Random-number generator for scheduling randomization */
	MyRNG rng_;
};

#define DEFINE_PURE_SCHEDULER_SUBCLASS(_policy, _base)                                             \
	class _policy##Scheduler : public _base {                                                  \
	public:                                                                                    \
		_policy##Scheduler(GenMCDriver *driver) : _base(driver) {}                         \
                                                                                                   \
	private:                                                                                   \
		virtual auto scheduleWithPolicy(const ExecutionGraph &g,                           \
						std::span<Action> runnable)                        \
			-> std::optional<int> override;                                            \
	};

DEFINE_PURE_SCHEDULER_SUBCLASS(LTR, Scheduler);
DEFINE_PURE_SCHEDULER_SUBCLASS(WF, Scheduler);
DEFINE_PURE_SCHEDULER_SUBCLASS(WFR, ArbitraryScheduler)

#endif /* GENMC_SCHEDULER_HPP */
