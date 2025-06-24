#ifndef GENMC_SCHEDULER_HPP
#define GENMC_SCHEDULER_HPP

#include "ADT/View.hpp"
#include "Config/Config.hpp"
#include "ExecutionGraph/ExecutionGraph.hpp"
#include "Runtime/InterpreterEnumAPI.hpp"

#include <optional>
#include <random>
#include <vector>

/** Determine whether a thread still has more events to replay and is not blocked. */
static auto isSchedulable(const ExecutionGraph &g, int thread) -> bool
{
	const auto *lab = g.getLastThreadLabel(thread);
	return !llvm::isa_and_nonnull<TerminatorLabel>(lab);
}

class Scheduler {
public:
	enum class Phase : std::uint8_t {
		TryOptimizeScheduling,
		Replay,
		Exploration,
	};

	Scheduler() = delete;
	virtual ~Scheduler() = default;

	static auto create(const Config *conf, bool estimationMode) -> std::unique_ptr<Scheduler>;

	auto getSchedulingPhase() const -> Phase { return phase_; }

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

	/** Opt: Check if we have any reads that were added blocked and whether we should reschedule
	 * them. */
	[[nodiscard]] auto checkRescheduleReads(ExecutionGraph &g) -> std::optional<int>;

	/** Opt: Whether the exploration should try to repair R */
	[[nodiscard]] auto isRescheduledRead(Event r) const -> bool
	{
		return readToReschedule_ == r;
	}

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
	void enterReplayPhase(const ExecutionGraph &g)
	{
		BUG_ON(phase_ != Scheduler::Phase::TryOptimizeScheduling);
		replaySchedule_.clear();
		calcPoRfReplay(g);
		phase_ = Scheduler::Phase::Replay;
	}

protected:
	Scheduler(const Config *conf) : conf_(conf) {}

private:
	/**
	 * Schedule the next thread according to a policy.
	 * Each subclass should provide this method with the specific policy
	 *  */
	[[nodiscard]] virtual auto scheduleWithPolicy(const ExecutionGraph &g,
						      std::span<Action> runnable)
		-> std::optional<int> = 0;

	/** Tries to schedule according to the current prioritization scheme */
	[[nodiscard]] auto schedulePrioritized(const ExecutionGraph &g) -> std::optional<int>;

	/** Opt: Tries to reschedule any reads that were added blocked */
	[[nodiscard]] auto rescheduleReads(ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int>;

	/**
	 * Pre-calculate a replay schedule for the given ExecutionGraph. The schedule will allow an
	 * interpreter to be brought up to a state corresponding to the ExecutionGraph.
	 *
	 * The replay will respect the (po U rf) relations in the graph, meaning it will run read
	 * events after the write events they read from.
	 * The replay is also WF (Writes-first), meaning it will attempt to schedule writes before
	 * reads when possible.
	 *
	 * Fully completed threads will be replayed iff `Config::replayCompletedThreads` is true.
	 */
	void calcPoRfReplay(const ExecutionGraph &g);
	void calcPoRfReplayRec(const EventLabel *lab, View &view);
	void finalizeReplaySchedule(const ExecutionGraph &g);

	auto getNextThreadToReplay(const ExecutionGraph &g, std::span<Action> runnable)
		-> std::optional<int>;

	[[nodiscard]] auto getConf() const -> const Config * { return conf_; }

	/** Opt: Whether a particular read needs to be repaired during rescheduling */
	Event readToReschedule_ = Event::getInit();

	/** Opt: Which thread(s) the scheduler should prioritize
	 * (empty if none) */
	std::vector<Event> threadPrios_{};

	const Config *conf_;

	/**
	 * Pre-calculated replay schedule containing positions that should be replayed in order.
	 * The thread of the event at the `back` of `replaySchedule_` is always the one to be
	 * replayed next, until the event is in the execution graph. Once the event is in the graph,
	 * it can be popped from the replay schedule. Depending on the interpreter, this may
	 * correspond to multiple calls to `scheduleNext`.
	 */
	std::vector<Event> replaySchedule_;

	Scheduler::Phase phase_{};
};

class ArbitraryScheduler : public Scheduler {
public:
	ArbitraryScheduler(const Config *conf) : Scheduler(conf)
	{
		/* Set up a random-number generator for the scheduler */
		std::random_device rd;
		auto seedVal =
			(!conf->randomScheduleSeed.empty())
				? static_cast<MyRNG::result_type>(stoull(conf->randomScheduleSeed))
				: rd();
		rng_.seed(seedVal);
	}

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
		_policy##Scheduler(const Config *conf) : _base(conf) {}                            \
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
