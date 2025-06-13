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

#ifdef ENABLE_GENMC_DEBUG
static void printReplaySchedule(std::span<Event> schedule)
{
	if (logLevel < VerbosityLevel::Tip)
		return;
	llvm::errs() << "Replay Schedule (" << schedule.size() << " entries): [";
	for (const auto &next : std::ranges::reverse_view(schedule)) {
		llvm::errs() << next << ", ";
	}
	llvm::errs() << "]\n";
}
#endif

void Scheduler::calcPoRfReplayRec(const ExecutionGraph &g, View &view, Event event)
{
	if (view.contains(event))
		return; /* Event already considered, skip... */

	auto i = view.getMax(event.thread);
	view.setMax(event);

	for (; i <= event.index; ++i) {
		const auto nextEvent = Event(event.thread, i);
		const auto *lab = g.getEventLabel(nextEvent);
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
			const auto *rfLab = rLab->getRf();
			if (rfLab && rfLab->getThread() != event.thread)
				calcPoRfReplayRec(g, view, rfLab->getPos());
		} else if (auto *jLab = llvm::dyn_cast<ThreadJoinLabel>(lab)) {
			const auto childId = jLab->getChildId();
			calcPoRfReplayRec(g, view, g.getLastThreadLabel(childId)->getPos());
		} else if (auto *bLab = llvm::dyn_cast<ThreadStartLabel>(lab)) {
			const auto createEvent = bLab->getCreateId();
			if (!createEvent.isInitializer())
				calcPoRfReplayRec(g, view, createEvent);
		}

		/* All po-rf-dependencies have been added to the replay schedule, so we can schedule
		 * this event now. */
		addReplayEvent(nextEvent);
	}
}

void Scheduler::finalizeReplaySchedule(const ExecutionGraph &g)
{
	if (replaySchedule_.empty())
		return;

	/* NOTE: the replay schedule is still in reverse order in the vector. */

	if (!getConf()->replayCompletedThreads) {
		/* Erase any non-schedulable threads. */
		std::erase_if(replaySchedule_,
			      [&g](const Event pos) { return !isSchedulable(g, pos.thread); });
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
		std::erase_if(replaySchedule_, [&g](const Event pos) {
			const auto *currLab = g.getEventLabel(pos);
			return currLab->isNotAtomic();
		});
	}

	/* Compress consecutive events for the same thread. */
	if (replaySchedule_.empty())
		return; /* We may have filtered out all events. */
	auto curr = replaySchedule_.begin();
	auto prev = curr;
	auto end = replaySchedule_.end();
	while (++curr != end) {
		if (curr->thread == prev->thread) {
			prev->index = std::max(prev->index, curr->index);
		} else {
			++prev;
			*prev = *curr;
		}
	}
	replaySchedule_.erase(++prev, end);

	/* The schedule is still reversed, need to fix that. */
	std::ranges::reverse(replaySchedule_);
}

void Scheduler::calcPoRfReplay(const ExecutionGraph &g)
{
	/* Linearize (po U rf) and use that as the replay schedule. */
	View view;
	const auto maxTid = static_cast<int>(g.getNumThreads());
	for (auto tid = 0; tid < maxTid; tid++) {
		const auto *lastLab = g.getLastThreadLabel(tid);
		calcPoRfReplayRec(g, view, lastLab->getPos());
	}

	finalizeReplaySchedule(g);

	/* Print the calculated replay schedule. */
	// GENMC_DEBUG(printReplaySchedule(replaySchedule_););
}

void Scheduler::resetExplorationOptions(const ExecutionGraph &g)
{
	phase_ = Scheduler::Phase::TryOptimizeScheduling;

	setRescheduledRead(Event::getInit());
	resetThreadPrioritization();
}

void Scheduler::addReplayEvent(Event event) { replaySchedule_.push_back(event); }

auto Scheduler::nextReplayThread(const ExecutionGraph &g, std::span<Action> runnable)
	-> std::optional<int>
{
	while (!replaySchedule_.empty()) {
		const Event next = replaySchedule_.back();

		// if ((next.thread >= runnable.size() || next.index == 0 ||
		if ((next.thread >= runnable.size() ||
		     next.index > runnable[next.thread].event.index) &&
		    isSchedulable(g, next.thread))
			return {getFirstSchedulableSymmetric(g, next.thread)};

		replaySchedule_.pop_back();
	}
	return {};
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
		if (auto next = nextReplayThread(g, runnable))
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
	if (result != threadPrios_.end())
		return {result->thread};
	return {}; /* No schedulable thread found */
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
	if (result != runnable.end())
		return {result->event.thread};
	return {}; /* No schedulable thread found */
}

/**** Specific Scheduling Policy Subclasses ****/

auto LTRScheduler::scheduleWithPolicy(const ExecutionGraph &g, std::span<Action> runnable)
	-> std::optional<int>
{
	auto result = std::ranges::find_if(runnable, [&g](const auto &action) {
		return isSchedulable(g, action.event.thread);
	});
	if (result != runnable.end())
		return {result->event.thread};
	return {}; /* No schedulable thread found */
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

	/* Otherwise, try to schedule the fallback thread */
	if (fallback != -1)
		return {getFirstSchedulableSymmetric(g, fallback)};

	return {}; /* No schedulable thread found */
}

auto RandomScheduler::scheduleWithPolicy(const ExecutionGraph &g, std::span<Action> runnable)
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

	/* No schedulable thread found */
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
