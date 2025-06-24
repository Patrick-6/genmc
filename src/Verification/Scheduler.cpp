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

void Scheduler::calcPoRfReplay(const ExecutionGraph &g)
{
	/* Calculate preliminary replay schedule (reversed order) */
	View view;
	for (auto i = 0U; i < g.getNumThreads(); i++)
		calcPoRfReplayRec(g.getLastThreadLabel(i), view);

	/* Erase any non-schedulable threads. */
	if (!getConf()->replayCompletedThreads) {
		std::erase_if(replaySchedule_, [&g](const auto &pos) {
			auto *lab = g.getLastThreadLabel(pos.thread);
			return !isSchedulable(g, lab->getThread()) &&
			       !llvm::isa_and_nonnull<BlockLabel>(lab);
		});
	}

	/* Erase NAs as they cannot affect the schedule (unless RD is disabled) */
	if (!getConf()->disableRaceDetection) {
		std::erase_if(replaySchedule_, [&g](const auto &pos) {
			return g.getEventLabel(pos)->isNotAtomic();
		});
	}

	/* The schedule is still reversed, need to fix that. */
	std::ranges::reverse(replaySchedule_);

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
	BUG_ON(phase_ == Phase::TryOptimizeScheduling);

	std::optional<int> result;
	if (phase_ == Phase::Replay && (result = getNextThreadToReplay(g, runnable)))
		return result;

	phase_ = Phase::Exploration;

	/* Check if we should prioritize some thread */
	if ((result = schedulePrioritized(g)))
		return result;

	/* Schedule the next thread according to the chosen policy. */
	if ((result = scheduleWithPolicy(g, runnable)))
		return result;

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
