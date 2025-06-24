#include "Verification/Scheduler.hpp"
#include "ExecutionGraph/Event.hpp"
#include "ExecutionGraph/EventLabel.hpp"
#include "ExecutionGraph/GraphUtils.hpp"
#include "Support/Error.hpp"

#include <llvm/Support/Casting.h>

#include <algorithm>
#include <ranges>
#include <span>
#include <string>
#include <vector>

auto Scheduler::create(const Config *conf, bool estimationMode) -> std::unique_ptr<Scheduler>
{
	if (estimationMode)
		return std::make_unique<WFRScheduler>(conf);

#define CREATE_SCHEDULER(_policy)                                                                  \
	case SchedulePolicy::_policy:                                                              \
		return std::make_unique<_policy##Scheduler>(conf)

	switch (conf->schedulePolicy) {
		CREATE_SCHEDULER(LTR);
		CREATE_SCHEDULER(WF);
		CREATE_SCHEDULER(WFR);
		CREATE_SCHEDULER(Arbitrary);
	default:
		BUG();
	}
}

static auto getFirstSchedulableSymmetric(const ExecutionGraph &g, int tid) -> int
{
	auto firstSched = tid;
	auto symm = g.getFirstThreadLabel(tid)->getSymmPredTid();
	while (symm != -1) {
		if (isSchedulable(g, symm))
			firstSched = symm;
		symm = g.getFirstThreadLabel(symm)->getSymmPredTid();
	}
	return firstSched;
}

void Scheduler::calcPoRfReplayRec(const EventLabel *lab, View &view)
{
	if (!lab || view.contains(lab->getPos()))
		return;

	auto i = view.getMax(lab->getThread());
	view.updateIdx(lab->getPos());

	const auto &g = *lab->getParent();
	for (++i; i <= lab->getIndex(); ++i) {
		const auto *pLab = g.getEventLabel(Event(lab->getThread(), i));
		if (const auto *rLab = llvm::dyn_cast<ReadLabel>(pLab))
			calcPoRfReplayRec(rLab->getRf(), view);
		else if (const auto *jLab = llvm::dyn_cast<ThreadJoinLabel>(pLab))
			calcPoRfReplayRec(g.getLastThreadLabel(jLab->getChildId()), view);
		else if (const auto *tsLab = llvm::dyn_cast<ThreadStartLabel>(pLab))
			calcPoRfReplayRec(tsLab->getCreate(), view);
		if (!llvm::isa<BlockLabel>(lab))
			replaySchedule_.push_back(pLab->getPos());
	}
}

void Scheduler::finalizeReplaySchedule(const ExecutionGraph &g)
{
	/* Erase any non-schedulable threads. */
	if (!getConf()->replayCompletedThreads) {
		std::erase_if(replaySchedule_, [&g](const auto &pos) {
			auto *lab = g.getLastThreadLabel(pos.thread);
			return !isSchedulable(g, lab->getThread()) &&
			       !llvm::isa_and_nonnull<BlockLabel>(lab);
		});
	}
	if (getConf()->onlyScheduleAtAtomics) {
		/* Erase any non-atomic replay events.
		 *
		 * Some interpreter frontends (e.g., Miri) can add multiple events when scheduled
		 * once. One example of this is `atomic_load`, which is treated like a special
		 * function call, so it can only be scheduled as one unit. It still creates multiple
		 * events for the graph:
		 *
		 * - Allocate any variables used in the function. 	(no dependencies)
		 * - NA read of the `ordering` from a constant.		(no dependencies)
		 * - Do the actual atomic load. 			  (possible dependencies)
		 * - Deallocate any variables used in the function. (no dependencies)
		 *
		 * Scheduling based on the first event potentially misses dependencies. Only
		 * scheduling at atomic events is sufficient to prevent this issue. */
		std::erase_if(replaySchedule_, [&g](const auto &pos) {
			return g.getEventLabel(pos)->isNotAtomic();
		});
	}

	/* The schedule is still reversed, need to fix that. */
	std::ranges::reverse(replaySchedule_);
}

void Scheduler::calcPoRfReplay(const ExecutionGraph &g)
{
	View view;
	for (auto i = 0U; i < g.getNumThreads(); i++)
		calcPoRfReplayRec(g.getLastThreadLabel(i), view);
	finalizeReplaySchedule(g);

	// GENMC_DEBUG(llvm::dbgs() << format(std::ranges::reverse_view(replaySchedule_)) << "\n";);
}

void Scheduler::resetExplorationOptions(const ExecutionGraph &g)
{
	phase_ = Scheduler::Phase::TryOptimizeScheduling;

	setRescheduledRead(Event::getInit());
	resetThreadPrioritization();
}

auto Scheduler::getNextThreadToReplay(const ExecutionGraph &g, std::span<Action> runnable)
	-> std::optional<int>
{
	/* Pop any events already replayed. This is done lazily as:
	 * 1. one interpreter instruction may map to many graph events (currently only for RMWs);
	 *    in such cases, schedule() is not called in between the events.
	 * 2. some runtimes may call schedule() before local instructions not tracked in the
	 *    graph (i.e., runnable remains the same across different calls) */
	while (!replaySchedule_.empty() &&
	       replaySchedule_.back().index <= runnable[replaySchedule_.back().thread].event.index)
		replaySchedule_.pop_back();

	if (replaySchedule_.empty())
		return {};

	/* If the next event to be replayed is an assume()-read, eagerly pop it. Otherwise,
	 * the instruction counter might never increase (due to the read blocking), and
	 * we will be stuck scheduling the same thread forever. */
	auto next = replaySchedule_.back();
	const auto *lastLab = g.getLastThreadLabel(next.thread);
	if (llvm::isa<BlockLabel>(lastLab) && next.index == lastLab->getIndex() - 1 &&
	    llvm::isa<ReadLabel>(g.po_imm_pred(lastLab))) /* overapproximation */
		replaySchedule_.pop_back();
	return {next.thread};
}

/**
 * Sets up the next thread to run in the interpreter
 * Will schedule according to the execution graph during replay,
 * then schedule according to the policy implemented in `scheduleWithPolicy`.
 *   */
auto Scheduler::scheduleNext(ExecutionGraph &g, std::span<Action> runnable) -> std::optional<int>
{
	/* Scheduling phase 2: Replay the ExecutionGraph by scheduling the interpreter. */
	BUG_ON(phase_ == Phase::TryOptimizeScheduling);
	if (phase_ == Phase::Replay) {
		if (auto next = getNextThreadToReplay(g, runnable))
			return next;

		phase_ = Phase::Exploration;
	}

	/* NOTE: At this point, `runnable.size() == g.getNumThread()` might not hold, since
	 * `runnable` might not contain some fully completed threads. */

	/* Scheduling phase 3: Normal execution by scheduling the interpreter repeatedly. */
	/* Check if we should prioritize some thread */
	if (auto next = schedulePrioritized(g))
		return next;

	/* Schedule the next thread according to the chosen policy. */
	if (auto next = scheduleWithPolicy(g, runnable))
		return next;

	/* All threads are either blocked or terminated, so we check if we can unblock some
	 * blocked reads. */
	return rescheduleReads(g, runnable);
}

auto Scheduler::schedulePrioritized(const ExecutionGraph &g) -> std::optional<int>
{
	if (threadPrios_.empty())
		return {};

	BUG_ON(getConf()->bound.has_value());

	auto result = std::ranges::find_if(
		threadPrios_, [&g](const auto &pos) { return isSchedulable(g, pos.thread); });
	return (result != threadPrios_.end()) ? std::make_optional(result->thread) : std::nullopt;
}

auto Scheduler::rescheduleReads(ExecutionGraph &g, std::span<Action> runnable) -> std::optional<int>
{
	auto result = std::ranges::find_if(runnable, [this, &g](const auto &action) {
		auto *bLab = llvm::dyn_cast_or_null<ReadOptBlockLabel>(
			g.getLastThreadLabel(action.event.thread));
		if (!bLab)
			return false;

		BUG_ON(getConf()->bound.has_value());
		setRescheduledRead(bLab->getPos());
		unblockThread(g, bLab->getPos());
		return true;
	});
	return (result != runnable.end()) ? std::make_optional(result->event.thread) : std::nullopt;
}

/**** Specific Scheduling Policy Subclasses ****/

auto LTRScheduler::scheduleWithPolicy(const ExecutionGraph &g, std::span<Action> runnable)
	-> std::optional<int>
{
	auto result = std::ranges::find_if(runnable, [&g](const auto &action) {
		return isSchedulable(g, action.event.thread);
	});
	return (result != runnable.end()) ? std::make_optional(result->event.thread) : std::nullopt;
}

auto WFScheduler::scheduleWithPolicy(const ExecutionGraph &g, std::span<Action> runnable)
	-> std::optional<int>
{
	/* Try and find a thread that has a non-load event next.
	 * Keep an LTR fallback option in case this fails. */
	int fallback = -1;
	auto result = std::ranges::find_if(runnable, [&g, &fallback](const auto &action) {
		if (!isSchedulable(g, action.event.thread))
			return false;
		if (fallback == -1)
			fallback = action.event.thread;
		return action.kind != ActionKind::Load;
	});

	if (result != runnable.end())
		return {getFirstSchedulableSymmetric(g, result->event.thread)};

	return (fallback != -1) ? std::make_optional(getFirstSchedulableSymmetric(g, fallback))
				: std::nullopt;
}

auto ArbitraryScheduler::scheduleWithPolicy(const ExecutionGraph &g, std::span<Action> runnable)
	-> std::optional<int>
{
	const auto numThreads = static_cast<int>(runnable.size());

	MyDist dist(0, numThreads);
	const auto random = static_cast<int>(dist(rng_));
	for (auto i = 0; i < numThreads; i++) {
		const auto &action = runnable[(i + random) % numThreads];

		if (!isSchedulable(g, action.event.thread))
			continue;

		/* Found a not-yet-complete thread; schedule it */
		return {getFirstSchedulableSymmetric(g, action.event.thread)};
	}
	return {};
}

auto WFRScheduler::scheduleWithPolicy(const ExecutionGraph &g, std::span<Action> runnable)
	-> std::optional<int>
{
	std::vector<int> nonwrites;
	std::vector<int> writes;
	for (auto &action : runnable) {
		if (!isSchedulable(g, action.event.thread))
			continue;

		if (action.kind != ActionKind::Load)
			writes.push_back(action.event.thread);
		else
			nonwrites.push_back(action.event.thread);
	}

	std::vector<int> &selection = !writes.empty() ? writes : nonwrites;
	if (selection.empty())
		return {};

	MyDist dist(0, selection.size() - 1);
	const auto candidate = selection[dist(getRng())];
	return {getFirstSchedulableSymmetric(g, candidate)};
}
