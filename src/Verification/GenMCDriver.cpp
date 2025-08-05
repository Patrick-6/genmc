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

#include "config.h"

#include "Config/Config.hpp"
#include "ExecutionGraph/Consistency/BoundDecider.hpp"
#include "ExecutionGraph/Consistency/ConsistencyChecker.hpp"
#include "ExecutionGraph/Consistency/SymmetryChecker.hpp"
#include "ExecutionGraph/DepExecutionGraph.hpp"
#include "ExecutionGraph/EventLabel.hpp"
#include "ExecutionGraph/GraphIterators.hpp"
#include "ExecutionGraph/GraphUtils.hpp"
#include "ExecutionGraph/LabelVisitor.hpp"
#include "GenMCDriver.hpp"
// #include "Runtime/Interpreter.h" // FIXME: remove interpreter dependency
#include "Static/LLVMModule.hpp"
#include "Support/DotPrint.hpp"
#include "Support/Error.hpp"
#include "Support/Logger.hpp"
#include "Support/Parser.hpp"
#include "Support/ResultHandling.hpp"
#include "Support/SExprVisitor.hpp"
#include "Support/SVal.hpp"
#include "Support/ThreadPool.hpp"
#include "Verification/DriverHandlerDispatcher.hpp"
#include "Verification/GenMCDriver.hpp"
#include "Verification/Relinche/LinearizabilityChecker.hpp"
#include "Verification/Scheduler.hpp"
#include "Verification/VerificationResult.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_os_ostream.h>

#include <algorithm>
#include <csignal>
#include <span>
#include <sstream>
#include <utility>

/************************************************************
 ** GENERIC MODEL CHECKING DRIVER
 ***********************************************************/

GenMCDriver::GenMCDriver(std::shared_ptr<const Config> conf, ThreadPool *pool /* = nullptr */,
			 Mode mode /* = VerificationMode{} */)
	: mode(mode), pool(pool), userConf(std::move(conf))
{
	/* Set up the execution context */
	auto initValGetter = [this](const auto &access) {
		ERROR("HACK: dummy initValGetter to remove EE dependency.");
		return SVal(0);
	};
	auto execGraph = userConf->isDepTrackingModel
				 ? std::make_unique<DepExecutionGraph>(initValGetter)
				 : std::make_unique<ExecutionGraph>(initValGetter);
	execStack.emplace_back(std::move(execGraph), std::move(LocalQueueT()),
			       std::move(ChoiceMap()), std::move(SAddrAllocator()),
			       Event::getInit());

	consChecker = ConsistencyChecker::create(getConf());
	symmChecker = SymmetryChecker::create();
	auto hasBounder = userConf->bound.has_value();
	GENMC_DEBUG(hasBounder |= userConf->boundsHistogram;);
	if (hasBounder)
		bounder = BoundDecider::create(getConf()->boundType);

	scheduler_ = inEstimationMode() ? std::make_unique<WFRScheduler>(getConf())
					: Scheduler::create(getConf());

	// TODO GENMC: can this be solved in a better way? Maybe only do this in estimation mode?
	/* Set up a random-number generator (for the scheduler) */
	std::random_device rd;
	auto seedVal = (!userConf->randomScheduleSeed.empty())
			       ? (MyRNG::result_type)stoull(userConf->randomScheduleSeed)
			       : rd();
	if (userConf->printRandomScheduleSeed) {
		PRINT(VerbosityLevel::Error) << "Estimation RNG Seed: " << seedVal << "\n";
	}
	estRng.seed(rd());

	/*
	 * Make sure we can resolve symbols in the program as well. We use 0
	 * as an argument in order to load the program, not a library. This
	 * is useful as it allows the executions of external functions in the
	 * user code.
	 */
	std::string ErrorStr;
	if (llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr, &ErrorStr)) {
		WARN("Could not resolve symbols in the program: " + ErrorStr);
	}

	if (userConf->collectLinSpec)
		result.specification = std::make_unique<Specification>(userConf->maxExtSize
#ifdef ENABLE_GENMC_DEBUG
								       ,
								       userConf->relincheDebug
#endif
		);
	if (userConf->checkLinSpec)
		relinche =
			LinearizabilityChecker::create(&getConsChecker(), *getConf()->checkLinSpec);
}

GenMCDriver::~GenMCDriver() = default;

GenMCDriver::Execution::Execution(std::unique_ptr<ExecutionGraph> g, LocalQueueT &&w, ChoiceMap &&m,
				  SAddrAllocator &&alloctor, Event lastAdded)
	: graph(std::move(g)), workqueue(std::move(w)), choices(std::move(m)),
	  alloctor(std::move(alloctor)), lastAdded(lastAdded)
{}
GenMCDriver::Execution::~Execution() = default;

static void repairRead(ExecutionGraph &g, ReadLabel *lab)
{
	auto *maxLab = g.co_max(lab->getAddr());
	lab->setRf(maxLab);
	lab->setAddedMax(true);
}

static void repairDanglingReads(ExecutionGraph &g)
{
	for (auto i = 0U; i < g.getNumThreads(); i++) {
		auto *rLab = llvm::dyn_cast<ReadLabel>(g.getLastThreadLabel(i));
		if (rLab && !rLab->getRf()) {
			repairRead(g, rLab);
		}
	}
}

static auto createAllocView(const ExecutionGraph &g) -> View
{
	View v;
	for (auto t : g.thr_ids()) {
		v.setMax({t, 1});
		for (auto &lab : g.po(t)) {
			auto *mLab = llvm::dyn_cast<MallocLabel>(&lab);
			if (!mLab)
				continue;
			if (mLab->getAllocAddr().isDynamic())
				v.setMax({t, static_cast<int>((mLab->getAllocAddr().get() &
							       SAddr::indexMask) +
							      mLab->getAllocSize())});
		}
	}
	return v;
}

void GenMCDriver::Execution::restrict(Stamp stamp)
{
	auto &g = getGraph();
	g.cutToStamp(stamp);
	repairDanglingReads(g);
	getChoiceMap().cut(*g.getViewFromStamp(stamp));
	getAllocator().restrict(createAllocView(g));
}

void GenMCDriver::pushExecution(Execution &&e) { execStack.push_back(std::move(e)); }

bool GenMCDriver::popExecution()
{
	if (execStack.empty())
		return false;
	execStack.pop_back();
	return !execStack.empty();
}

void GenMCDriver::initFromState(std::unique_ptr<Execution> exec,
				ExecutionGraph::InitValGetter initValGetter)
{
	execStack.clear();
	execStack.emplace_back(std::move(exec->graph), LocalQueueT(), std::move(exec->choices),
			       std::move(exec->alloctor), exec->lastAdded);

	/* We have to also reset the initvalgetter */
	getExec().getGraph().setInitValGetter(std::move(initValGetter));
}

std::unique_ptr<GenMCDriver::Execution> GenMCDriver::extractState()
{
	return std::make_unique<Execution>(GenMCDriver::Execution(
		getExec().getGraph().clone(), LocalQueueT(), ChoiceMap(getExec().getChoiceMap()),
		SAddrAllocator(getExec().getAllocator()), getExec().getLastAdded()));
}

/* Returns a fresh address to be used from the interpreter */
static auto getFreshAddr(const MallocLabel *aLab, SAddrAllocator &alloctor) -> SAddr
{
	/* The arguments to getFreshAddr() need to be well-formed;
	 * make sure the alignment is positive and a power of 2 */
	auto alignment = aLab->getAlignment();
	BUG_ON(alignment <= 0 || (alignment & (alignment - 1)) != 0);
	switch (aLab->getStorageDuration()) {
	case StorageDuration::SD_Automatic:
		return alloctor.allocAutomatic(aLab->getThread(), aLab->getAllocSize(), alignment,
					       aLab->getStorageType() == StorageType::ST_Durable,
					       aLab->getAddressSpace() ==
						       AddressSpace::AS_Internal);
	case StorageDuration::SD_Heap:
		return alloctor.allocHeap(aLab->getThread(), aLab->getAllocSize(), alignment,
					  aLab->getStorageType() == StorageType::ST_Durable,
					  aLab->getAddressSpace() == AddressSpace::AS_Internal);
	case StorageDuration::SD_Static: /* Cannot ask for fresh static addresses */
	default:
		BUG();
	}
	BUG();
	return {};
}

void GenMCDriver::handleExecutionStart()
{
	/* Set various exploration options for this execution */
	unmoot();
	getScheduler().resetExplorationOptions(getExec().getGraph());
}

void GenMCDriver::checkHelpingCasAnnotation()
{
	/* If we were waiting for a helped CAS that did not appear, complain */
	auto &g = getExec().getGraph();
	for (auto i = 0U; i < g.getNumThreads(); i++) {
		if (llvm::isa<HelpedCASBlockLabel>(g.getLastThreadLabel(i)))
			ERROR("Helped/Helping CAS annotation error! Does helped CAS always "
			      "execute?\n");
	}

	/* Next, we need to check whether there are any extraneous
	 * stores, not visible to the helped/helping CAS */
	for (auto &lab : g.labels() | std::views::filter([](auto &lab) {
				 return llvm::isa<HelpingCasLabel>(&lab);
			 })) {
		auto *hLab = llvm::dyn_cast<HelpingCasLabel>(&lab);

		/* Check that all stores that would make this helping
		 * CAS succeed are read by a helped CAS.
		 * We don't need to check the swap value of the helped CAS */
		if (std::any_of(g.co_begin(hLab->getAddr()), g.co_end(hLab->getAddr()),
				[&](auto &sLab) {
					return hLab->getExpected() == sLab.getVal() &&
					       std::none_of(
						       sLab.readers_begin(), sLab.readers_end(),
						       [&](auto &rLab) {
							       return llvm::isa<HelpedCasReadLabel>(
								       &rLab);
						       });
				}))
			ERROR("Helped/Helping CAS annotation error! "
			      "Unordered store to helping CAS location!\n");

		/* Special case for the initializer (as above) */

		//    TODO GENMC: pass required info (initial value of access)
		if (hLab->getAddr().isStatic() &&
		    hLab->getExpected() == g.getInitVal(hLab->getAccess())) {
			auto rsView = g.labels() | std::views::filter([hLab](auto &lab) {
					      auto *rLab = llvm::dyn_cast<ReadLabel>(&lab);
					      return rLab && rLab->getAddr() == hLab->getAddr();
				      });
			if (std::ranges::none_of(rsView, [&](auto &lab) {
				    return llvm::isa<HelpedCasReadLabel>(&lab);
			    }))
				ERROR("Helped/Helping CAS annotation error! "
				      "Unordered store to helping CAS location!\n");
		}
	}
	return;
}

#ifdef ENABLE_GENMC_DEBUG
void GenMCDriver::trackExecutionBound()
{
	auto bound = bounder->calculate(getExec().getGraph());
	result.exploredBounds.grow(bound);
	result.exploredBounds[bound]++;
}
#endif

bool GenMCDriver::isExecutionBlocked(std::span<Action> runnable) const
{
	const auto &g = getExec().getGraph();
	for (const auto &action : runnable) {
		const auto tid = action.event.thread;
		if (tid < g.getNumThreads() && !g.isThreadEmpty(tid) &&
		    llvm::isa_and_nonnull<BlockLabel>(g.getLastThreadLabel(tid))) {
			return true;
		}
	}
	return false;
}

void GenMCDriver::updateStSpaceEstimation()
{
	/* Calculate current sample */
	auto &choices = getExec().getChoiceMap();
	auto sample = std::accumulate(choices.begin(), choices.end(), 1.0L,
				      [](auto sum, auto &kv) { return sum *= kv.second.size(); });

	/* This is the (i+1)-th exploration */
	auto totalExplored = (long double)result.explored + result.exploredBlocked + 1L;

	/* As the estimation might stop dynamically, we can't just
	 * normalize over the max samples to avoid overflows. Instead,
	 * use Welford's online algorithm to calculate mean and
	 * variance. */
	auto prevM = result.estimationMean;
	auto prevV = result.estimationVariance;
	result.estimationMean += (sample - prevM) / totalExplored;
	result.estimationVariance +=
		(sample - prevM) / totalExplored * (sample - result.estimationMean) -
		prevV / totalExplored;
}

static const auto maybeTimeRelinche = [](auto &&relinche, auto &&g) {
#ifdef ENABLE_GENMC_DEBUG
	const auto &start = std::chrono::high_resolution_clock::now();
#endif
	auto res = relinche.refinesSpec(g);
#ifdef ENABLE_GENMC_DEBUG
	const auto &stop = std::chrono::high_resolution_clock::now();
	res.analysisTime = stop - start;
#endif
	return res;
};

std::unique_ptr<ModelCheckerError> GenMCDriver::handleExecutionEnd(std::span<Action> runnable)
{
	MIRI_LOG() << "GenMCDriver::handleExecutionEnd (isHalting: " << isHalting()
		   << ", isMoot: " << isMoot() << ")\n";
	if (isMoot()) {
		GENMC_DEBUG(++result.exploredMoot;);
		return std::make_unique<ModelCheckerError>(
			"TODO GENMC: execution is moot"); // TODO GENMC: should this return
							  // something?
	}

	/* Helper: Check helping CAS annotation */
	if (getConf()->helper)
		checkHelpingCasAnnotation();

	/* If under estimation mode, guess the total.
	 * (This may run a few times, but that's OK.)*/
	if (inEstimationMode()) {
		updateStSpaceEstimation();
		if (!shouldStopEstimating())
			getExec().getWorkqueue().add(std::make_unique<RerunForwardRevisit>());
	}

	/* Ignore the execution if some assume has failed */
	if (isExecutionBlocked(runnable)) {
		MIRI_LOG() << "EXECUTION IS BLOCKED! " /* << runnable */ << "\n";
		++result.exploredBlocked;
		if (getConf()->printBlockedExecs)
			printGraph();
		if (getConf()->checkLiveness)
			checkLiveness(); // TODO GENMC: what should be returned here here?
		return {};		 // TODO GENMC: is it correct to return no error here?
	}

	if (getConf()->warnUnfreedMemory)
		checkUnfreedMemory(); // TODO GENMC (WARNINGS): how to handle warnings?
	if (getConf()->printExecGraphs)
		printGraph();
	GENMC_DEBUG(if (getConf()->boundsHistogram && !inEstimationMode()) trackExecutionBound(););

	++result.explored;
	if (fullExecutionExceedsBound())
		++result.boundExceeding;

	if (isHalting() || isMoot()) {
		// TODO GENMC: what should be returned here?
		return std::make_unique<ModelCheckerError>(
			"TODO GENMC: execution is halting, blocked, or moot");
	}

	if (inEstimationMode())
		return {};

	/* Relinche: Collect/check abstract behavior */
	if (getConf()->collectLinSpec)
		result.specification->add(getExec().getGraph(), &getConsChecker(),
					  getConf()->symmetryReduction);
	if (getConf()->checkLinSpec) {
		result.relincheResult += maybeTimeRelinche(getRelinche(), getExec().getGraph());
		if (result.relincheResult.status) {
			result.status = VerificationError::VE_LinearizabilityError;
			auto msg = result.relincheResult.status->toString();
			auto ret_result = std::make_unique<ModelCheckerError>(msg);
			reportError({Event::getBottom(), result.status, msg});
			return ret_result;
		}
	}
	return {};
}

bool GenMCDriver::done()
{

	auto validExecution = false;
	while (!isHalting() && !validExecution) {
		auto item = getExec().getWorkqueue().getNext();
		if (!item) {
			if (popExecution())
				continue;
			return true;
		}
		validExecution = restrictAndRevisit(item) && isRevisitValid(*item);
	}
	return isHalting();
}

bool GenMCDriver::isHalting() const
{
	auto *tp = getThreadPool();
	return shouldHalt || (tp && tp->shouldHalt());
}

void GenMCDriver::halt(VerificationError status)
{
	MIRI_LOG() << "GenMC::halt called! error: " << (uint64_t)status << "\n";
	shouldHalt = true;
	result.status = status;
	if (getThreadPool())
		getThreadPool()->halt();
}

/************************************************************
 ** Scheduling methods
 ***********************************************************/

void GenMCDriver::blockThreadTryMoot(std::unique_ptr<BlockLabel> bLab)
{
	auto &g = getExec().getGraph();
	auto pos = bLab->getPos();
	blockThread(g, std::move(bLab));
	auto *lab = g.getLastThreadLabel(pos.thread);
	mootExecutionIfFullyBlocked(lab);
}

auto GenMCDriver::scheduleNext(std::span<Action> runnable) -> std::optional<int>
{
	return (isMoot() || isHalting()) ? std::nullopt
					 : getScheduler().schedule(getExec().getGraph(), runnable);
}

auto GenMCDriver::runFromCache() -> bool
{
	if (!getConf()->instructionCaching || inEstimationMode())
		return false;

	do {
		auto toAdd = getScheduler().scheduleFromCache(getExec().getGraph());
		if (!toAdd)
			return true;
		if (*toAdd == nullptr)
			return false;

		addLabelsToGraph(**toAdd);
	} while (!isMoot() && !isHalting());
	return true;
}

bool isUninitializedAccess(const SAddr &addr, const Event &pos)
{
	return addr.isDynamic() && pos.isInitializer();
}

bool GenMCDriver::isExecutionValid(const EventLabel *lab)
{
	return (!getConf()->symmetryReduction || getSymmChecker().isSymmetryOK(lab)) &&
	       getConsChecker().isConsistent(lab) && !partialExecutionExceedsBound();
}

bool GenMCDriver::isRevisitValid(const Revisit &revisit)
{
	auto &g = getExec().getGraph();
	auto pos = revisit.getPos();
	auto *mLab = llvm::dyn_cast<MemAccessLabel>(g.getEventLabel(pos));

	/* E.g., for optional revisits, do nothing */
	if (!mLab)
		return true;

	if (!isExecutionValid(mLab))
		return false;

	/* If an extra event is added, re-check consistency */
	auto *rLab = llvm::dyn_cast<ReadLabel>(mLab);
	auto *nLab = g.po_imm_succ(mLab);
	return !rLab || !rLab->isRMW() ||
	       (isExecutionValid(nLab) && checkForRaces(nLab) == VerificationError::VE_OK);
}

bool GenMCDriver::isExecutionDrivenByGraph(const EventLabel *lab)
{
	const auto &g = getExec().getGraph();
	auto curr = lab->getPos();
	return (curr.index < g.getThreadSize(curr.thread)) &&
	       !llvm::isa<EmptyLabel>(g.getEventLabel(curr));
}

bool GenMCDriver::executionExceedsBound(BoundCalculationStrategy strategy) const
{
	if (!getConf()->bound.has_value() || inEstimationMode())
		return false;

	return bounder->doesExecutionExceedBound(getExec().getGraph(), *getConf()->bound, strategy);
}

bool GenMCDriver::fullExecutionExceedsBound() const
{
	return executionExceedsBound(BoundCalculationStrategy::NonSlacked);
}

bool GenMCDriver::partialExecutionExceedsBound() const
{
	return executionExceedsBound(BoundCalculationStrategy::Slacked);
}

auto GenMCDriver::inReplay(const Event e) const -> bool
{
	// return getEE()->getExecState() == llvm::ExecutionState::Replay;
	MIRI_LOG() << "TODO GENMC: GenMCDriver::inReplay: currently always returning `false`\n";
	return false;
}

EventLabel *GenMCDriver::addLabelToGraph(std::unique_ptr<EventLabel> lab)
{
	MIRI_LOG() << "GenMCDriver::addLabelToGraph, label " << *lab
		   << ", event: " << lab->getPos().thread << ", " << lab->getPos().index << "\n";

	auto &g = getExec().getGraph();

	/* We should not add events to a blocked/terminated thread. */
	if (lab->getThread() < g.getNumThreads() && !g.isThreadEmpty(lab->getThread()) &&
	    llvm::isa<TerminatorLabel>(g.getLastThreadLabel(lab->getThread()))) [[unlikely]] {
		LOG(VerbosityLevel::Error) << "GenMCDriver::addLabelToGraph: trying to add label "
					   << *lab << " to blocked thread with last label: "
					   << *g.getLastThreadLabel(lab->getThread()) << "\n";
		BUG();
	}

	if (g.containsPos(lab->getPos())) {
		LOG(VerbosityLevel::Error)
			<< "Adding label " << *lab
			<< " to graph, but there is already a label at that position: "
			<< *g.getEventLabel(lab->getPos()) << "\n";
		BUG();
	}

	/* Cache the event before updating views (inits are added w/ tcreate) */
	if (getConf()->instructionCaching && !inEstimationMode())
		getScheduler().cacheEventLabel(g, &*lab);

	/* Add and update views */
	auto *addedLab = g.addLabelToGraph(std::move(lab));
	updateLabelViews(addedLab);
	if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(addedLab))
		g.addAlloc(findAllocatingLabel(g, mLab->getAddr()), mLab);
	getExec().getLastAdded() = addedLab->getPos();
	if (addedLab->getIndex() >= getConf()->warnOnGraphSize) {
		LOG_ONCE("large-graph", VerbosityLevel::Tip)
			<< "The execution graph seems quite large. "
			<< "Consider bounding all loops or using -unroll\n";
	}
	return addedLab;
}

void GenMCDriver::addLabelsToGraph(const std::vector<std::unique_ptr<EventLabel>> &labs)
{
	auto &g = getExec().getGraph();
	DriverHandlerDispatcher dispatcher(this);

	for (const auto &vlab : labs) {
		BUG_ON(vlab->hasStamp());

		dispatcher.visit(vlab);

		if (isMoot() || llvm::isa<BlockLabel>(g.getLastThreadLabel(vlab->getThread())))
			break;
	}

	/* Graph well-formedness: ensure RMWs events are scheduled as one.
	 * (Cannot rely on next round scheduling the same thread.) */
	auto *lastLab = g.getEventLabel(getExec().getLastAdded());
	if (auto *rLab = llvm::dyn_cast<ReadLabel>(lastLab)) {
		if (auto wLab = createRMWWriteLabel(g, rLab))
			dispatcher.visit(*wLab);
	}
}

void GenMCDriver::updateLabelViews(EventLabel *lab)
{
	getConsChecker().updateMMViews(lab);
	if (!getConf()->symmetryReduction)
		return;

	getSymmChecker().updatePrefixWithSymmetries(lab);
}

VerificationError GenMCDriver::checkForRaces(const EventLabel *lab)
{
	if (getConf()->disableRaceDetection || inEstimationMode())
		return VerificationError::VE_OK;

	/* Check for hard errors */
	const EventLabel *racyLab = nullptr;
	auto err = getConsChecker().checkErrors(lab, racyLab);
	if (err != VerificationError::VE_OK) {
		MIRI_LOG()
			<< "GenMCDriver::checkForRaces: getConsChecker().checkErrors found error: "
			<< (uint64_t)err << " == \"" << err << "\"\n";
		reportError({lab->getPos(), err, "", racyLab});
		return err;
	}

	/* Check whether there are any unreported warnings... */
	std::vector<const EventLabel *> races;
	auto newWarnings = getConsChecker().checkWarnings(lab, getResult().warnings, races);

	/* ... and report them */
	auto i = 0U;
	for (auto &wcode : newWarnings) {
		if (reportWarningOnce(lab->getPos(), wcode, races[i++]))
			return wcode;
	}
	return VerificationError::VE_OK;
}

LoadResult GenMCDriver::getReadRetValue(const ReadLabel *rLab)
{
	/* Bottom is an acceptable re-option only @ replay */
	if (!rLab->getRf()) {
		BUG_ON(!inReplay(rLab->getPos()));
		// TODO GENMC: what is the correct error here? (if it even is an error)
		return LoadResult::fromError("ReadLabel has no Reads-From label");
	}

	using Evaluator = SExprEvaluator<ModuleID::ID>;
	auto res = rLab->getAccessValue(rLab->getAccess());
	auto &g = getExec().getGraph();

	/* Return nullopt for reads that require an interpreter reset */
	if (getConf()->ipr && rLab->getAnnot() &&
	    !Evaluator().evaluate(&*rLab->getAnnot()->expr, res)) {
		blockThread(g, BlockLabel::createAssumeBlock(rLab->getPos().next(),
							     rLab->getAnnot()->type));

		// TODO GENMC: what is the correct error here? (if it even is an error)
		return LoadResult::fromError("??? (No read return value)");
	}
	if (llvm::isa<BWaitReadLabel>(rLab) &&
	    !readsBarrierUnblockingValue(llvm::cast<BWaitReadLabel>(rLab))) {
		blockThread(g, BlockLabel::createAssumeBlock(rLab->getPos().next(),
							     AssumeType::Barrier));
		// TODO GENMC: what is the correct error here? (if it even is an error)
		return LoadResult::fromError("??? (BWaitReadLabel)");
	}
	return LoadResult::fromValue(res);
}

SVal GenMCDriver::getRecReadRetValue(const ReadLabel *rLab)
{
	auto &g = getExec().getGraph();

	/* Find and read from the latest sameloc store */
	auto preds = po_preds(g, rLab);
	auto wLabIt = std::ranges::find_if(preds, [rLab](auto &lab) {
		auto *wLab = llvm::dyn_cast<WriteLabel>(&lab);
		return wLab && wLab->getAddr() == rLab->getAddr();
	});
	BUG_ON(wLabIt == std::ranges::end(preds));
	return (*wLabIt).getAccessValue(rLab->getAccess());
}

VerificationError GenMCDriver::checkAccessValidity(const MemAccessLabel *lab)
{
	/* Some interpreters check for access validity already, skip in that case. */
	if (getConf()->skipAccessValidityChecks)
		return VerificationError::VE_OK;

	/* Static variable validity is handled by the interpreter. *
	 * Dynamic accesses are valid if they access allocated memory */
	if ((!lab->getAddr().isDynamic() && !getEE()->isStaticallyAllocated(lab->getAddr())) ||
	    (lab->getAddr().isDynamic() && !lab->getAlloc())) {
		reportError({lab->getPos(), VerificationError::VE_AccessNonMalloc});
		return VerificationError::VE_AccessNonMalloc;
	}
	return VerificationError::VE_OK;
}

VerificationError GenMCDriver::checkInitializedMem(const ReadLabel *rLab)
{
	/* Slightly unrelated check, but ensure there are no mixed-size accesses. */
	/* Skip checking for mixed-size accesses between two non-atomic accesses depending on
	 * config. */
	if (rLab->getRf() &&
	    (!getConf()->allowNonAtomicMixedSizeAccesses ||
	     rLab->getOrdering() != MemOrdering::NotAtomic ||
	     rLab->getRf()->getOrdering() != MemOrdering::NotAtomic) &&
	    !rLab->getRf()->getPos().isInitializer() &&
	    llvm::dyn_cast<WriteLabel>(rLab->getRf())->getSize() != rLab->getSize()) {
		reportError({rLab->getPos(), VerificationError::VE_MixedSize,
			     "Mixed-size accesses detected: tried to read with a " +
				     std::to_string(rLab->getSize().get() * 8) + "-bit access!\n" +
				     "Please check the LLVM-IR.\n"});
		return VerificationError::VE_MixedSize;
	}

	/* Some interpreters check for access validity already, skip in that case. */
	if (getConf()->skipAccessValidityChecks)
		return VerificationError::VE_OK;

	// FIXME: Have label for mutex-destroy and check type instead of val.
	//        Also for barriers.

	/* Locks should not read from destroyed mutexes */
	const auto *lLab = llvm::dyn_cast<LockCasReadLabel>(rLab);
	if (lLab && lLab->getAccessValue(lLab->getAccess()) == SVal(-1)) {
		reportError({lLab->getPos(), VerificationError::VE_UninitializedMem,
			     "Called lock() on destroyed mutex!", lLab->getRf()});
		return VerificationError::VE_UninitializedMem;
	}

	/* Plain events should read initialized memory if they are dynamic accesses */
	// TODO GENMC(uninit checks): this check likely needs to be upgraded, maybe to actually
	// check initValGetter or something (maybe easiest to add `is_init` to `SVal`)
	// TODO GENMC: Q0: what happens when trying to read an uninitialized global? (Is this case
	// impossible in C/C++? It could happeen in Rust with `MaybeUninit`)
	// TODO GENMC: Q0: what happens when trying to read an initialized local? (so no reads,
	// meaning getRf()->getPos() is the initializer event?)
	if (isUninitializedAccess(rLab->getAddr(), rLab->getRf()->getPos())) {
		MIRI_LOG() << "TODO GENMC: checkInitializedMem: read from uninitialized memory ("
			   << rLab->getAddr() << ", " << rLab->getAddr().get()
			   << ")! pos: " << rLab->getRf()->getThread() << ", "
			   << rLab->getRf()->getIndex() << "\n";
		reportError({rLab->getPos(), VerificationError::VE_UninitializedMem});
		return VerificationError::VE_UninitializedMem;
	}
	return VerificationError::VE_OK;
}

VerificationError GenMCDriver::checkInitializedMem(const WriteLabel *wLab)
{
	/* Some interpreters check for access validity already, skip in that case. */
	if (getConf()->skipAccessValidityChecks)
		return VerificationError::VE_OK;

	auto &g = getExec().getGraph();

	/* Unlocks should unlock mutexes locked by the same thread */
	const auto *uLab = llvm::dyn_cast<UnlockWriteLabel>(wLab);
	if (uLab && !findMatchingLock(uLab)) {
		reportError({uLab->getPos(), VerificationError::VE_InvalidUnlock,
			     "Called unlock() on mutex not locked by the same thread!"});
		return VerificationError::VE_InvalidUnlock;
	}
	return VerificationError::VE_OK;
}

VerificationError GenMCDriver::checkFinalAnnotations(const WriteLabel *wLab)
{
	if (!getConf()->finalWrite)
		return VerificationError::VE_OK;

	auto &g = getExec().getGraph();

	if (g.hasLocMoreThanOneStore(wLab->getAddr()))
		return VerificationError::VE_OK;
	if ((wLab->isFinal() &&
	     std::any_of(g.co_begin(wLab->getAddr()), g.co_end(wLab->getAddr()),
			 [&](auto &sLab) {
				 return !getConsChecker().getHbView(wLab).contains(sLab.getPos());
			 })) ||
	    (!wLab->isFinal() && std::any_of(g.co_begin(wLab->getAddr()), g.co_end(wLab->getAddr()),
					     [&](auto &sLab) { return sLab.isFinal(); }))) {
		reportError({wLab->getPos(), VerificationError::VE_Annotation,
			     "Multiple stores at final location!"});
		return VerificationError::VE_Annotation;
	}
	return VerificationError::VE_OK;
}

VerificationError GenMCDriver::checkIPRValidity(const ReadLabel *rLab)
{
	if (!rLab->getAnnot() || !getConf()->ipr)
		return VerificationError::VE_OK;

	auto &g = getExec().getGraph();
	auto racyIt = std::find_if(g.co_begin(rLab->getAddr()), g.co_end(rLab->getAddr()),
				   [&](auto &wLab) { return wLab.hasAttr(WriteAttr::WWRacy); });
	if (racyIt == g.co_end(rLab->getAddr()))
		return VerificationError::VE_OK;

	auto msg = "Unordered writes do not constitute a bug per se, though they often "
		   "indicate faulty design.\n"
		   "This warning is treated as an error due to in-place revisiting (IPR).\n"
		   "You can use -disable-ipr to disable this feature."s;
	reportError({racyIt->getPos(), VerificationError::VE_WWRace, msg, nullptr, true});
	return VerificationError::VE_WWRace;
}

bool threadReadsMaximal(const ExecutionGraph &g, int tid)
{
	/*
	 * Depending on whether this is a DSA loop or not, we have to
	 * adjust the detection starting point: DSA-blocked threads
	 * will have a SpinStart as their last event.
	 */
	BUG_ON(!llvm::isa<BlockLabel>(g.getLastThreadLabel(tid)));
	auto *lastLab = g.po_imm_pred(g.getLastThreadLabel(tid));
	auto start = llvm::isa<SpinStartLabel>(lastLab) ? lastLab->getPos().prev()
							: lastLab->getPos();
	for (auto j = start.index; j > 0; j--) {
		auto *lab = g.getEventLabel(Event(tid, j));
		BUG_ON(llvm::isa<LoopBeginLabel>(lab));
		if (llvm::isa<SpinStartLabel>(lab))
			return true;
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
			if (rLab->getRf() != g.co_max(rLab->getAddr()))
				return false;
		}
	}
	BUG();
}

void GenMCDriver::checkLiveness()
{
	if (isHalting())
		return;

	const auto &g = getExec().getGraph();

	/* Collect all threads blocked at spinloops */
	std::vector<int> spinBlocked;
	for (auto i = 0U; i < g.getNumThreads(); i++) {
		if (llvm::isa<SpinloopBlockLabel>(g.getLastThreadLabel(i)))
			spinBlocked.push_back(i);
	}

	if (spinBlocked.empty())
		return;

	/* And check whether all of them are live or not */
	auto nonTermTID = 0u;
	if (std::all_of(spinBlocked.begin(), spinBlocked.end(), [&](int tid) {
		    nonTermTID = tid;
		    return threadReadsMaximal(g, tid);
	    })) {
		/* Print some TID blocked by a spinloop */
		reportError({g.getLastThreadLabel(nonTermTID)->getPos(),
			     VerificationError::VE_Liveness,
			     "Non-terminating spinloop: thread " + std::to_string(nonTermTID)});
	}
	return;
}

void GenMCDriver::checkUnfreedMemory()
{
	if (isHalting())
		return;

	auto &g = getExec().getGraph();
	const MallocLabel *unfreedAlloc = nullptr;
	if (std::ranges::any_of(g.labels(), [&](auto &lab) {
		    unfreedAlloc = llvm::dyn_cast<MallocLabel>(&lab);
		    return unfreedAlloc && unfreedAlloc->getFree() == nullptr;
	    })) {
		reportWarningOnce(unfreedAlloc->getPos(), VerificationError::VE_UnfreedMemory);
	}
}

void GenMCDriver::filterConflictingBarriers(const ReadLabel *lab, std::vector<EventLabel *> &stores)
{
	if (getConf()->disableBAM ||
	    (!llvm::isa<BIncFaiReadLabel>(lab) && !llvm::isa<BWaitReadLabel>(lab)))
		return;

	/* Helper lambdas */
	auto isReadByExclusiveRead = [&](auto *oLab) {
		if (auto *wLab = llvm::dyn_cast<WriteLabel>(oLab))
			return std::ranges::any_of(wLab->readers(),
						   [&](auto &rLab) { return rLab.isRMW(); });
		if (auto *iLab = llvm::dyn_cast<InitLabel>(oLab))
			return std::ranges::any_of(iLab->rfs(lab->getAddr()),
						   [&](auto &rLab) { return rLab.isRMW(); });
		BUG();
	};
	auto findFaiReader = [](BIncFaiWriteLabel *wLab) {
		return std::find_if(wLab->readers_begin(), wLab->readers_end(),
				    [](auto &rLab) { return llvm::isa<BIncFaiReadLabel>(&rLab); });
	};
	auto findSameRoundMaximal = [&](BIncFaiWriteLabel *wLab) {
		auto &g = *wLab->getParent();
		while (!isLastInBarrierRound(wLab) && findFaiReader(wLab) != wLab->readers_end()) {
			wLab = llvm::dyn_cast<BIncFaiWriteLabel>(
				g.po_imm_succ(&*findFaiReader(wLab)));
		}
		return wLab;
	};

	/* barrier_wait()'s plain load should read maximally */
	if (auto *rLab = llvm::dyn_cast<BWaitReadLabel>(lab)) {
		auto *wLab = llvm::dyn_cast<BIncFaiWriteLabel>(stores[0]);
		BUG_ON(!wLab || wLab->getPos().next() != lab->getPos());
		stores[0] = findSameRoundMaximal(wLab);
		stores.resize(1);
		return;
	}

	/* barrier_wait()'s FAI loads should not read from conflicting stores */
	stores.erase(std::remove_if(stores.begin(), stores.end(),
				    [&](auto &sLab) { return isReadByExclusiveRead(sLab); }),
		     stores.end());
}

void GenMCDriver::filterSymmetricStoresSR(const ReadLabel *rLab,
					  std::vector<EventLabel *> &stores) const
{
	auto &g = getExec().getGraph();
	auto t = g.getFirstThreadLabel(rLab->getThread())->getSymmPredTid();

	/* If there is no symmetric thread, exit */
	if (t == -1)
		return;

	/* Check whether the po-prefixes of the two threads match */
	if (!getSymmChecker().sharePrefixSR(t, rLab))
		return;

	/* Get the symmetric event and make sure it matches as well */
	auto *lab = llvm::dyn_cast<ReadLabel>(g.getEventLabel(Event(t, rLab->getIndex())));
	if (!lab || lab->getAddr() != rLab->getAddr() || lab->getSize() != lab->getSize())
		return;

	if (!lab->isRMW())
		return;

	/* Remove stores that will be explored symmetrically */
	auto rfStamp = lab->getRf()->getStamp();
	stores.erase(std::remove_if(
			     stores.begin(), stores.end(),
			     [&](auto &sLab) { return lab->getRf()->getPos() == sLab->getPos(); }),
		     stores.end());
}

void GenMCDriver::filterValuesFromAnnotSAVER(const ReadLabel *rLab,
					     std::vector<EventLabel *> &validStores)
{
	/* Locks are treated as annotated CASes */
	if (!rLab->getAnnot())
		return;

	using Evaluator = SExprEvaluator<ModuleID::ID>;

	auto &g = getExec().getGraph();

	/* Ensure we keep the maximal store around even if Helper messed with it */
	BUG_ON(validStores.empty());
	auto maximal = validStores.back();
	validStores.erase(std::remove_if(validStores.begin(), validStores.end(),
					 [&](auto *wLab) {
						 auto val = wLab->getAccessValue(rLab->getAccess());
						 return wLab != maximal &&
							wLab != g.co_max(rLab->getAddr()) &&
							!Evaluator().evaluate(
								&*rLab->getAnnot()->expr, val);
					 }),
			  validStores.end());
	BUG_ON(validStores.empty());
}

void GenMCDriver::unblockWaitingHelping(const WriteLabel *lab)
{
	if (!llvm::isa<HelpedCasWriteLabel>(lab))
		return;

	/* FIXME: We have to wake up all threads waiting on helping CASes,
	 * as we don't know which ones are from the same CAS */
	for (auto i = 0u; i < getExec().getGraph().getNumThreads(); i++) {
		auto *bLab = llvm::dyn_cast_or_null<HelpedCASBlockLabel>(
			getExec().getGraph().getLastThreadLabel(i));
		if (bLab)
			getExec().getGraph().removeLast(bLab->getThread());
	}
}

bool GenMCDriver::writesBeforeHelpedContainedInView(const HelpedCasReadLabel *lab, const View &view)
{
	auto &g = getExec().getGraph();
	auto &hb = getConsChecker().getHbView(lab);

	for (auto i = 0u; i < hb.size(); i++) {
		auto j = hb.getMax(i);
		while (!llvm::isa<WriteLabel>(g.getEventLabel(Event(i, j))) && j > 0)
			--j;
		if (j > 0 && !view.contains(Event(i, j)))
			return false;
	}
	return true;
}

bool GenMCDriver::checkHelpingCasCondition(const HelpingCasLabel *hLab)
{
	auto &g = getExec().getGraph();

	auto hsView = g.labels() | std::views::filter([&g, hLab](auto &lab) {
			      auto *rLab = llvm::dyn_cast<HelpedCasReadLabel>(&lab);
			      return rLab && rLab->isRMW() && rLab->getAddr() == hLab->getAddr() &&
				     rLab->getType() == hLab->getType() &&
				     rLab->getSize() == hLab->getSize() &&
				     rLab->getOrdering() == hLab->getOrdering() &&
				     rLab->getExpected() == hLab->getExpected() &&
				     rLab->getSwapVal() == hLab->getSwapVal();
		      });

	if (std::ranges::any_of(hsView, [&g, this](auto &lab) {
		    auto *hLab = llvm::dyn_cast<HelpedCasReadLabel>(&lab);
		    auto &view = getConsChecker().getHbView(hLab);
		    return !writesBeforeHelpedContainedInView(hLab, view);
	    }))
		ERROR("Helped/Helping CAS annotation error! "
		      "Not all stores before helped-CAS are visible to helping-CAS!\n");
	return std::ranges::begin(hsView) != std::ranges::end(hsView);
}

EventLabel *GenMCDriver::findConsistentRf(ReadLabel *rLab, std::vector<EventLabel *> &rfs)
{
	auto &g = getExec().getGraph();

	/* For the non-bounding case, maximal extensibility guarantees consistency */
	if (!getConf()->bound.has_value()) {
		auto *back = rfs.back();
		rfs.pop_back();
		rLab->setRf(back);
		return back;
	}

	/* Otherwise, search for a consistent rf */
	while (!rfs.empty()) {
		auto *back = rfs.back();
		rfs.pop_back();
		rLab->setRf(back);
		if (isExecutionValid(rLab))
			return back;
	}

	/* Extensibility is guaranteed with bounding because
	 * - the consistent choice might lead to a settled atomicity violation and has already been
	 * filtered-out
	 * - context bounding's slack might have changed due to an optimization */
	BUG_ON(!getConf()->bound.has_value() ||
	       (getConf()->boundType != BoundType::context && !llvm::isa<CasReadLabel>(rLab) &&
		!llvm::isa<FaiReadLabel>(rLab)));
	return nullptr;
}

EventLabel *GenMCDriver::findConsistentCo(WriteLabel *wLab, std::vector<EventLabel *> &cos)
{
	auto &g = getExec().getGraph();

	/* Similarly to the read case: rely on extensibility */
	auto back = cos.back();
	wLab->addCo(back);
	if (!getConf()->bound.has_value()) {
		cos.pop_back();
		return back;
	}

	// FIXME: This is wrong
	/* In contrast to the read case, we need to be a bit more careful:
	 * the consistent choice might not satisfy atomicity, but we should
	 * keep it around to try revisits */
	while (!cos.empty()) {
		auto back = cos.back();
		cos.pop_back();
		wLab->moveCo(back);
		if (isExecutionValid(wLab))
			return back;
	}
	return nullptr;
}

void GenMCDriver::handleThreadKill(std::unique_ptr<ThreadKillLabel> kLab)
{
	if (isExecutionDrivenByGraph(&*kLab)) {
		BUG_ON(!inReplay(kLab->getPos())); // TODO GENMC: fix/change
		return;
	}
	addLabelToGraph(std::move(kLab));
}

bool GenMCDriver::isSymmetricToSR(int candidate, Event parent, const ThreadInfo &info) const
{
	auto &g = getExec().getGraph();
	auto cParent = g.getFirstThreadLabel(candidate)->getCreateId();
	auto &cInfo = g.getFirstThreadLabel(candidate)->getThreadInfo();

	/* A tip to print to the user in case two threads look
	 * symmetric, but we cannot deem it */
	auto tipSymmetry = [&]() {
		LOG_ONCE("possible-symmetry", VerbosityLevel::Tip)
			<< "Threads " << cInfo.id << " and " << info.id
			<< " could benefit from symmetry reduction."
			<< " Consider using __VERIFIER_spawn_symmetric().\n";
	};

	/* First, check that the two threads are actually similar */
	if (cInfo.id == info.id || cInfo.parentId != info.parentId || cInfo.funId != info.funId ||
	    cInfo.arg != info.arg) {
		if (cInfo.funId == info.funId && cInfo.parentId == info.parentId)
			tipSymmetry();
		return false;
	}

	/* Then make sure that there is no memory access in between the spawn events */
	auto mm = std::minmax(parent.index, cParent.index);
	auto minI = mm.first;
	auto maxI = mm.second;
	for (auto j = minI; j < maxI; j++) {
		if (llvm::isa<MemAccessLabel>(g.getEventLabel(Event(parent.thread, j)))) {
			tipSymmetry();
			return false;
		}
	}
	return true;
}

int GenMCDriver::getSymmetricTidSR(const ThreadCreateLabel *tcLab,
				   const ThreadInfo &childInfo) const
{
	if (!getConf()->symmetryReduction)
		return -1;

	/* Has the user provided any info? */
	if (childInfo.symmId != -1)
		return childInfo.symmId;

	auto &g = getExec().getGraph();

	for (auto i = childInfo.id - 1; i > 0; i--)
		if (isSymmetricToSR(i, tcLab->getPos(), childInfo))
			return i;
	return -1;
}

auto GenMCDriver::handleThreadCreate(std::unique_ptr<ThreadCreateLabel> tcLab)
	-> ThreadCreateLabel *
{
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*tcLab)) {
		return llvm::dyn_cast<ThreadCreateLabel>(g.getEventLabel(tcLab->getPos()));
	}

	/* First, check if the thread to be created already exists */
	int cid = 0;
	while (cid < static_cast<int>(g.getNumThreads())) {
		if (!g.isThreadEmpty(cid)) {
			auto *bLab = llvm::dyn_cast_or_null<ThreadStartLabel>(
				g.getFirstThreadLabel(cid));
			if (bLab && bLab->getCreateId() == tcLab->getPos())
				break;
		}
		++cid;
	}

	/* Add an event for the thread creation */
	tcLab->setChildId(cid);
	auto *lab = llvm::dyn_cast<ThreadCreateLabel>(addLabelToGraph(std::move(tcLab)));

	/* This tid should not already exist in the graph */
	BUG_ON(cid != (long)g.getNumThreads());
	g.addNewThread();

	/* Create a label and add it to the graph; is the thread symmetric to another one? */
	auto symm = getSymmetricTidSR(lab, lab->getChildInfo());
	auto *tsLab = addLabelToGraph(ThreadStartLabel::create(Event(cid, 0), lab->getPos(), lab,
							       lab->getChildInfo(), symm));
	if (symm != -1)
		g.getFirstThreadLabel(symm)->setSymmSuccTid(cid);
	return lab;
}

LoadResult GenMCDriver::handleThreadJoin(std::unique_ptr<ThreadJoinLabel> lab)
{
	MIRI_LOG() << "GenMCDriver::handleThreadJoin: " << *lab << "\n";
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*lab)) {
		MIRI_LOG() << "GenMCDriver::handleThreadJoin: label in graph\n";
		return LoadResult::fromValue(g.getEventLabel(lab->getPos())->getReturnValue());
	}

	if (!llvm::isa_and_nonnull<ThreadFinishLabel>(g.getLastThreadLabel(lab->getChildId()))) {
		blockThread(g, JoinBlockLabel::create(lab->getPos(), lab->getChildId()));
		MIRI_LOG() << "GenMCDriver::handleThreadJoin: child NOT yet finished\n";
		return LoadResult::fromError(
			"Thread is blocked"); // TODO GENMC: maybe rename `LoadResult`, and possibly
					      // use enums here instead of strings
	}

	MIRI_LOG() << "GenMCDriver::handleThreadJoin: child IS finished\n";
	auto *jLab = llvm::dyn_cast<ThreadJoinLabel>(addLabelToGraph(std::move(lab)));
	auto cid = jLab->getChildId();

	auto *eLab = llvm::dyn_cast<ThreadFinishLabel>(g.getLastThreadLabel(cid));
	BUG_ON(!eLab);
	eLab->setParentJoin(jLab);

	if (cid < 0 || long(g.getNumThreads()) <= cid || cid == jLab->getThread()) {
		std::string err = "ERROR: Invalid TID in pthread_join(): " + std::to_string(cid);
		if (cid == jLab->getThread())
			err += " (TID cannot be the same as the calling thread)";
		reportError({jLab->getPos(), VerificationError::VE_InvalidJoin, err});
		return LoadResult::fromValue(SVal(0));
	}

	if (partialExecutionExceedsBound()) {
		moot();
		return LoadResult::fromError("Partial Execution Exceeds Bound");
	}

	return LoadResult::fromValue(jLab->getReturnValue());
}

void GenMCDriver::handleThreadFinish(std::unique_ptr<ThreadFinishLabel> eLab)
{
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*eLab))
		return;

	auto *lab = addLabelToGraph(std::move(eLab));
	for (auto i = 0U; i < g.getNumThreads(); i++) {
		auto *pLab = llvm::dyn_cast_or_null<JoinBlockLabel>(g.getLastThreadLabel(i));
		if (pLab && pLab->getChildId() == lab->getThread()) {
			/* If parent thread is waiting for me, relieve it */
			unblockThread(g, pLab->getPos());
		}
	}
	if (partialExecutionExceedsBound())
		moot();
}

void GenMCDriver::handleFence(std::unique_ptr<FenceLabel> fLab)
{
	if (isExecutionDrivenByGraph(&*fLab))
		return;

	addLabelToGraph(std::move(fLab));
}

void GenMCDriver::checkReconsiderFaiSpinloop(const MemAccessLabel *lab)
{
	auto &g = getExec().getGraph();

	for (auto i = 0u; i < g.getNumThreads(); i++) {
		/* Is there any thread blocked on a potential spinloop? */
		auto *eLab = llvm::dyn_cast_or_null<FaiZNEBlockLabel>(g.getLastThreadLabel(i));
		if (!eLab)
			continue;

		/* Check whether this access affects the spinloop variable */
		auto epreds = po_preds(g, eLab);
		auto faiLabIt = std::ranges::find_if(
			epreds, [](auto &lab) { return llvm::isa<FaiWriteLabel>(&lab); });
		BUG_ON(faiLabIt == std::ranges::end(epreds));

		auto *faiLab = llvm::dyn_cast<FaiWriteLabel>(&*faiLabIt);
		if (faiLab->getAddr() != lab->getAddr())
			continue;

		/* FAIs on the same variable are OK... */
		if (llvm::isa<FaiReadLabel>(lab) || llvm::isa<FaiWriteLabel>(lab))
			continue;

		/* If it does, and also breaks the assumptions, unblock thread */
		if (!getConsChecker().getHbView(faiLab).contains(lab->getPos())) {
			auto pos = eLab->getPos();
			unblockThread(g, pos);
			addLabelToGraph(FaiZNESpinEndLabel::create(pos));
		}
	}
}

const VectorClock &GenMCDriver::getPrefixView(const EventLabel *lab) const
{
	// FIXME
	if (!lab->hasPrefixView())
		lab->setPrefixView(const_cast<ConsistencyChecker &>(getConsChecker())
					   .calculatePrefixView(lab));
	return lab->getPrefixView();
}

std::vector<EventLabel *> GenMCDriver::getRfsApproximation(ReadLabel *lab)
{
	auto &g = getExec().getGraph();
	auto &cc = getConsChecker();
	auto rfs = cc.getCoherentStores(lab);
	if (!llvm::isa<CasReadLabel>(lab) && !llvm::isa<FaiReadLabel>(lab))
		return rfs;

	/* Remove atomicity violations */
	auto &before = getPrefixView(lab);
	auto isSettledRMWInView = [&before](auto &rLab) {
		auto &g = *rLab.getParent();
		return rLab.isRMW() && (!rLab.isRevisitable() || before.contains(rLab.getPos()));
	};
	auto atomicityViolationInView = [&isSettledRMWInView, lab](auto *sLab) {
		if (auto *wLab = llvm::dyn_cast<WriteLabel>(sLab)) {
			return lab->valueMakesRMWSucceed(wLab->getVal()) &&
			       std::ranges::any_of(wLab->readers(), isSettledRMWInView);
		};

		auto *iLab = llvm::cast<InitLabel>(sLab);
		auto addr = lab->getAddr();
		/* Reads to dynamic addresses cannot have read from Init */
		return !addr.isDynamic() &&
		       lab->valueMakesRMWSucceed(iLab->getAccessValue(lab->getAccess())) &&
		       std::ranges::any_of(iLab->rfs(addr), isSettledRMWInView);
	};
	rfs.erase(std::remove_if(rfs.begin(), rfs.end(), atomicityViolationInView), rfs.end());
	return rfs;
}

void GenMCDriver::filterOptimizeRfs(const ReadLabel *lab, std::vector<EventLabel *> &stores)
{
	/* Symmetry reduction */
	if (getConf()->symmetryReduction)
		filterSymmetricStoresSR(lab, stores);

	/* BAM */
	if (!getConf()->disableBAM)
		filterConflictingBarriers(lab, stores);

	/* Keep values that do not lead to blocking */
	filterValuesFromAnnotSAVER(lab, stores);
}

void GenMCDriver::filterAtomicityViolations(const ReadLabel *rLab,
					    std::vector<EventLabel *> &stores)
{
	auto &g = getExec().getGraph();
	if (!llvm::isa<CasReadLabel>(rLab) && !llvm::isa<FaiReadLabel>(rLab))
		return;

	const auto *casLab = llvm::dyn_cast<CasReadLabel>(rLab);
	auto valueMakesSuccessfulRMW = [&casLab, rLab](auto &&val) {
		return !casLab || val == casLab->getExpected();
	};
	stores.erase(
		std::remove_if(
			stores.begin(), stores.end(),
			[&](auto *sLab) {
				if (auto *iLab = llvm::dyn_cast<InitLabel>(sLab))
					return std::any_of(
						iLab->rf_begin(rLab->getAddr()),
						iLab->rf_end(rLab->getAddr()), [&](auto &rLab) {
							return rLab.isRMW() &&
							       valueMakesSuccessfulRMW(
								       rLab.getAccessValue(
									       rLab.getAccess()));
						});
				return std::any_of(
					rf_succ_begin(g, sLab), rf_succ_end(g, sLab),
					[&](auto &rLab) {
						return rLab.isRMW() &&
						       valueMakesSuccessfulRMW(rLab.getAccessValue(
							       rLab.getAccess()));
					});
			}),
		stores.end());
}

EventLabel *GenMCDriver::pickRandomRf(ReadLabel *rLab, std::vector<EventLabel *> &stores)
{
	auto &g = getExec().getGraph();

	stores.erase(std::remove_if(stores.begin(), stores.end(),
				    [&](auto &sLab) {
					    rLab->setRf(sLab);
					    return !isExecutionValid(rLab);
				    }),
		     stores.end());

	/* There is no bounding during estimation; reads are always extensible */
	BUG_ON(stores.empty());

	MyDist dist(0, stores.size() - 1);
	auto random = dist(estRng);
	rLab->setRf(stores[random]);
	return stores[random];
}

LoadResult GenMCDriver::handleLoad(std::unique_ptr<ReadLabel> rLab,
				   std::function<void(SAddr)> oldValSetter)
{
	VerificationError err;
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*rLab)) {
		MIRI_LOG() << "\tReadLabel isExecutionDriverByGraph is true\n";
		if (oldValSetter) {
			oldValSetter(rLab->getAddr());
		}
		return getReadRetValue(llvm::dyn_cast<ReadLabel>(g.getEventLabel(rLab->getPos())));
	}

	auto *lab = llvm::dyn_cast<ReadLabel>(addLabelToGraph(std::move(rLab)));

	if ((err = checkAccessValidity(lab)) != VerificationError::VE_OK) {
		// TODO GENMC: use Verification Result instead
		// std::ostringstream s;
		std::string buf;
		llvm::raw_string_ostream s(buf);
		return LoadResult::fromError(s.str()); // This execution will be blocked
	}
	if ((err = checkForRaces(lab)) != VerificationError::VE_OK) {
		// TODO GENMC: better error reporting (precise error would be
		// nice)
		// std::ostringstream s;
		std::string buf;
		llvm::raw_string_ostream s(buf);
		s << "TODO GENMC: checkForRaces failed: " << err;
		return LoadResult::fromError(s.str()); // This execution will be blocked
	}
	if ((err = checkIPRValidity(lab)) != VerificationError::VE_OK) {
		// TODO GENMC: use Verification Result instead
		// std::ostringstream s;
		std::string buf;
		llvm::raw_string_ostream s(buf);
		s << "TODO GENMC: checkIPRValidity failed: " << err;
		return LoadResult::fromError(s.str()); // This execution will be blocked
	}

	/* Check whether the load forces us to reconsider some existing event */
	checkReconsiderFaiSpinloop(lab);

	if (oldValSetter) {
		oldValSetter(lab->getAddr());
	}

	/* If a CAS read cannot be added maximally, reschedule */
	if (!getScheduler().isRescheduledRead(lab->getPos()) &&
	    removeCASReadIfBlocks(lab, g.co_max(lab->getAddr())))
		// TODO GENMC: what is the correct return here?
		return LoadResult();
	if (getScheduler().isRescheduledRead(lab->getPos()))
		getScheduler().setRescheduledRead(Event::getInit());

	/* Get an approximation of the stores we can read from */
	auto stores = getRfsApproximation(lab);
	BUG_ON(stores.empty());
	GENMC_DEBUG(LOG(VerbosityLevel::Debug3) << "Rfs: " << format(stores) << "\n";);
	filterOptimizeRfs(lab, stores);
	GENMC_DEBUG(LOG(VerbosityLevel::Debug3) << "Rfs (optimized): " << format(stores) << "\n";);

	EventLabel *rf = nullptr;
	if (inEstimationMode()) {
		getExec().getChoiceMap().update(lab, stores);
		filterAtomicityViolations(lab, stores);
		rf = pickRandomRf(lab, stores);
	} else {
		rf = findConsistentRf(lab, stores);
		/* Push all the other alternatives choices to the Stack */
		for (const auto &sLab : stores) {
			getExec().getWorkqueue().add(std::make_unique<ReadForwardRevisit>(
				lab->getPos(), sLab->getPos()));
		}
	}

	// TODO GENMC: call oldValSetter here?

	if (!rf)
		moot();

	/* Ensured the selected rf comes from an initialized memory location */
	if (!rf)
		return LoadResult::fromError("Memory Load: reads-from label has no value");
	if ((err = checkInitializedMem(lab)) != VerificationError::VE_OK) {
		if (err == VerificationError::VE_UninitializedMem) {
			return LoadResult::fromError("Memory Load: read from uninitialized memory");
		}
		if (err == VerificationError::VE_MixedSize) {
			// TODO GENMC: give more details:
			return LoadResult::fromError("Memory Load: mixed size read");
		}
		ERROR("invalid error type: " << err << "\n");
	}

	GENMC_DEBUG(LOG(VerbosityLevel::Debug2) << "--- Added load " << lab->getPos() << "\n"
						<< getExec().getGraph(););

	return getReadRetValue(lab);
}

static auto getRevisitable(WriteLabel *sLab, const VectorClock &before) -> std::vector<ReadLabel *>
{
	auto &g = *sLab->getParent();
	std::vector<ReadLabel *> loads;

	/* Helper function to erase loads in between conflicting RMWs */
	auto eraseConflictingLoads = [](auto &sLab, auto &loads) {
		auto *confLab = findPendingRMW(sLab);
		if (!confLab)
			return;
		loads.erase(std::remove_if(loads.begin(), loads.end(),
					   [&](auto &eLab) {
						   return eLab->getStamp() > confLab->getStamp();
					   }),
			    loads.end());
	};

	/* Fastpath: previous co-max is ppo-before SLAB */
	auto prevCoMaxIt = std::find_if(g.co_rbegin(sLab->getAddr()), g.co_rend(sLab->getAddr()),
					[&](auto &lab) { return lab.getPos() != sLab->getPos(); });
	if (prevCoMaxIt != g.co_rend(sLab->getAddr()) && before.contains(prevCoMaxIt->getPos())) {
		for (auto &rLab : prevCoMaxIt->readers()) {
			if (!rLab.isStable() && !before.contains(rLab.getPos()))
				loads.push_back(&rLab);
		}
		eraseConflictingLoads(sLab, loads);
		return loads;
	}

	/* Slowpath: iterate over all same-location reads */
	for (auto it = ++ExecutionGraph::reverse_label_iterator(sLab);
	     it != ExecutionGraph::reverse_label_iterator(g.getInitLabel()); ++it) {
		auto *rLab = llvm::dyn_cast<ReadLabel>(&*it);
		if (rLab && rLab->getAddr() == sLab->getAddr() && !rLab->isStable() &&
		    !before.contains(rLab->getPos()))
			loads.push_back(rLab);
	}
	eraseConflictingLoads(sLab, loads);
	return loads;
}

std::vector<ReadLabel *> GenMCDriver::getRevisitableApproximation(WriteLabel *sLab)
{
	auto &g = getExec().getGraph();
	const auto &prefix = getPrefixView(sLab);
	auto loads = getRevisitable(sLab, prefix);
	getConsChecker().filterCoherentRevisits(sLab, loads);
	std::ranges::sort(loads, [&g](auto &lab1, auto &lab2) {
		return lab1->getStamp() > lab2->getStamp();
	});
	return loads;
}

EventLabel *GenMCDriver::pickRandomCo(WriteLabel *sLab, std::vector<EventLabel *> &cos)
{
	auto &g = getExec().getGraph();

	sLab->addCo(cos.back());
	cos.erase(std::remove_if(cos.begin(), cos.end(),
				 [&](auto &wLab) {
					 sLab->moveCo(wLab);
					 return !isExecutionValid(sLab);
				 }),
		  cos.end());

	/* Extensibility is not guaranteed if an RMW read is not reading maximally
	 * (during estimation, reads read from arbitrary places anyway).
	 * If that is the case, we have to ensure that estimation won't stop. */
	if (cos.empty()) {
		BUG_ON(!sLab->isRMW());
		getExec().getWorkqueue().add(std::make_unique<RerunForwardRevisit>());
		return nullptr;
	}

	MyDist dist(0, cos.size() - 1);
	auto random = dist(estRng);
	sLab->moveCo(cos[random]);
	return cos[random];
}

void GenMCDriver::calcCoOrderings(WriteLabel *lab, const std::vector<EventLabel *> &cos)
{
	for (auto &predLab : cos) {
		getExec().getWorkqueue().add(
			std::make_unique<WriteForwardRevisit>(lab->getPos(), predLab->getPos()));
	}
}

StoreResult GenMCDriver::handleStore(std::unique_ptr<WriteLabel> wLab,
				     std::function<void(SAddr)> oldValSetter)
{
	VerificationError err{};
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*wLab)) {
		// TODO GENMC: add a boolean to indicate whether write is
		// maximal in the graph (and Miri should thus also do it)
		MIRI_LOG() << "GenMCDriver::handleStore A: getting co_max for position: "
			   << wLab->getPos() << "\n";
		if (oldValSetter) {
			MIRI_LOG() << "handleStore: calling oldValSetter A\n";
			oldValSetter(wLab->getAddr());
		}
		const bool isCoMaxWrite = g.co_max(wLab->getAddr())->getPos() == wLab->getPos();
		return StoreResult::ok(isCoMaxWrite);
	}

	auto *lab = llvm::dyn_cast<WriteLabel>(addLabelToGraph(std::move(wLab)));

	/* Stores cannot cause atomicity violation:
	 * - In normal mode, non-maximal RMW are completed elsewhere
	 * - In estimation mode, we have already filtered violations on the read part */
	// TODO GENMC (ERROR HANDLING)
	if ((err = checkAccessValidity(lab)) != VerificationError::VE_OK) {
		MIRI_LOG() << "handleStore found an access validity error: " << err << "\n";
		return StoreResult::fromError(
			"memory store: AccessValidityError (TODO GENMC: add more info here)");
	}
	if ((err = checkInitializedMem(lab)) != VerificationError::VE_OK) {
		MIRI_LOG() << "handleStore found an initialized memory error: " << err << "\n";
		return StoreResult::fromError(
			"memory store: InitializedMemError (TODO GENMC: add more info here)");
	}
	if ((err = checkFinalAnnotations(lab)) != VerificationError::VE_OK) {
		MIRI_LOG() << "handleStore found an final annotations error: " << err << "\n";
		// TODO GENMC: what is this?
		return StoreResult::fromError(
			"memory store: FinalAnnotationsError (TODO GENMC: add more info here)");
	}
	if ((err = checkForRaces(lab)) != VerificationError::VE_OK) {
		MIRI_LOG() << "TODO GENMC: handleStore found a data race error " << err << "\n";
		// TODO GENMC: use Verification Result instead
		// std::ostringstream s;
		std::string buf;
		llvm::raw_string_ostream s(buf);
		s << "memory store: Data Race Error (TODO GENMC: add more info here), error: "
		  << err;
		return StoreResult::fromError(s.str()); // This execution will be blocked
	}

	checkReconsiderFaiSpinloop(lab);
	unblockWaitingHelping(lab);
	checkReconsiderReadOpts(lab);

	if (oldValSetter) {
		MIRI_LOG() << "handleStore: calling oldValSetter\n";
		oldValSetter(lab->getAddr());
	}

	/* Find all possible placings in coherence for this store, and
	 * print a WW-race warning if appropriate (if this moots,
	 * exploration will anyway be cut) */

	auto cos = getConsChecker().getCoherentPlacings(lab);
	if (cos.size() > 1) {
		// TODO GENMC (ERROR HANDLING): how to handle warnigs?
		reportWarningOnce(lab->getPos(), VerificationError::VE_WWRace, cos[0]);
	}

	EventLabel *co = nullptr;
	if (inEstimationMode()) {
		co = pickRandomCo(lab, cos);
		getExec().getChoiceMap().update(lab, cos);
	} else {
		co = findConsistentCo(lab, cos);
		calcCoOrderings(lab, cos);
	}

	GENMC_DEBUG(LOG(VerbosityLevel::Debug2) << "--- Added store " << lab->getPos() << "\n"
						<< getExec().getGraph(););

	if (inReplay(lab->getPos())) {
		MIRI_LOG()
			<< "GenMCDriver::handleStore: inRecoveryMode or inReplay, returning `ok`\n"
			<< "GenMCDriver::handleStore B: getting co_max for position: "
			<< lab->getPos() << "\n";
		const bool isCoMaxWrite = g.co_max(lab->getAddr())->getPos() == lab->getPos();
		return StoreResult::ok(isCoMaxWrite); // TODO GENMC (ERROR HANDLING)
	}

	// TODO GENMC: should we use the return value of this?:
	calcRevisits(lab);
	if (!co || violatesAtomicity(lab))
		moot();

	MIRI_LOG() << "GenMCDriver::handleStore C: getting co_max for position: " << lab->getPos()
		   << "\n";
	const bool isCoMaxWrite = g.co_max(lab->getAddr())->getPos() == lab->getPos();
	return StoreResult::ok(isCoMaxWrite); // TODO GENMC (ERROR HANDLING)
}

SAddr GenMCDriver::handleMalloc(std::unique_ptr<MallocLabel> aLab)
{
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*aLab)) {
		auto *lab = llvm::dyn_cast<MallocLabel>(g.getEventLabel(aLab->getPos()));
		BUG_ON(!lab);
		return SAddr(lab->getAllocAddr().get());
	}

	/* Fix and add label to the graph. Cached labels might already have an address,
	 * but we enforce that's the same with the new one dispensed (for non-dep-tracking) */
	auto oldAddr = aLab->getAllocAddr();
	BUG_ON(!getConf()->isDepTrackingModel && oldAddr != SAddr() &&
	       oldAddr != aLab->getAllocAddr());
	if (oldAddr == SAddr())
		aLab->setAllocAddr(getFreshAddr(&*aLab, getExec().getAllocator()));
	const auto *lab = llvm::dyn_cast<MallocLabel>(addLabelToGraph(std::move(aLab)));
	return SAddr(lab->getAllocAddr().get());
}

void GenMCDriver::handleFree(std::unique_ptr<FreeLabel> dLab)
{
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*dLab))
		return;

	/* Find the size of the area deallocated */
	auto size = 0u;
	auto alloc = findAllocatingLabel(g, dLab->getFreedAddr());
	if (alloc) {
		size = alloc->getAllocSize();
	}
	BUG_ON(!alloc);

	/* Add a label with the appropriate store */
	dLab->setFreedSize(size);
	dLab->setAlloc(alloc);
	auto *lab = addLabelToGraph(std::move(dLab));
	alloc->setFree(llvm::dyn_cast<FreeLabel>(lab));

	/* Check whether there is any memory race */
	if (checkForRaces(lab) != VerificationError::VE_OK) {
		MIRI_LOG() << "TODO GENMC: handleFree found a race, need to handle this somehow\n";
	}
}

const MemAccessLabel *GenMCDriver::getPreviousVisibleAccessLabel(const EventLabel *start) const
{
	auto &g = getExec().getGraph();
	std::vector<Event> finalReads;

	for (const auto &lab : g.po_preds(start)) {
		if (auto *rLab = llvm::dyn_cast<ReadLabel>(&lab)) {
			if (rLab->isConfirming())
				continue;
			if (rLab->getRf()) {
				auto *wLab = llvm::dyn_cast<WriteLabel>(rLab->getRf());
				if (wLab && wLab->isLocal())
					continue;
				if (wLab && wLab->isFinal()) {
					finalReads.push_back(rLab->getPos());
					continue;
				}
				if (std::any_of(finalReads.begin(), finalReads.end(),
						[&](const Event &l) {
							auto *lLab = llvm::dyn_cast<ReadLabel>(
								g.getEventLabel(l));
							return lLab->getAddr() == rLab->getAddr() &&
							       lLab->getSize() == rLab->getSize();
						}))
					continue;
			}
			return rLab;
		}
		if (auto *wLab = llvm::dyn_cast<WriteLabel>(&lab))
			if (!wLab->isFinal() && !wLab->isLocal())
				return wLab;
	}
	return nullptr; /* none found */
}

void GenMCDriver::mootExecutionIfFullyBlocked(EventLabel *bLab)
{
	// TODO GENMC (ERROR HANDLING): return result if execution is now moot (e.g., bool or enum)
	auto &g = getExec().getGraph();

	auto *lab = getPreviousVisibleAccessLabel(bLab);
	if (auto *rLab = llvm::dyn_cast_or_null<ReadLabel>(lab))
		if (!rLab->isRevisitable() || !rLab->wasAddedMax())
			moot();
	return;
}

void GenMCDriver::handleBlock(std::unique_ptr<BlockLabel> lab)
{
	if (isExecutionDrivenByGraph(&*lab))
		return;

	/* Call addLabelToGraph first to cache the label */
	addLabelToGraph(lab->clone());
	// TODO GENMC: add result return value (e.g., if now mooted).
	blockThreadTryMoot(std::move(lab));
}

std::unique_ptr<VectorClock> GenMCDriver::getReplayView() const
{
	auto &g = getExec().getGraph();
	auto v = g.getViewFromStamp(g.getMaxStamp());

	/* handleBlock() is usually only called during normal execution
	 * and hence not reproduced during replays.
	 * We have to remove BlockLabels so that these will not lead
	 * to the execution of extraneous instructions */
	for (auto i = 0u; i < g.getNumThreads(); i++)
		if (llvm::isa<BlockLabel>(g.getLastThreadLabel(i)))
			v->setMax(Event(i, v->getMax(i) - 1));
	return v;
}

void GenMCDriver::reportError(const ErrorDetails &details)
{
	MIRI_LOG() << "TODO GENMC: properly handle errors: msg: '" << details.msg << "', ("
		   << details.pos.thread << ", " << details.pos.index << "), " << details.racyLab
		   << ", " << details.shouldHalt << ", " << details.type << "\n";

	auto &g = getExec().getGraph();

	/* If we have already detected an error, no need to report another */
	if (isHalting())
		return;

	/* If we this is a replay (might happen if one LLVM instruction
	 * maps to many MC events), do not get into an infinite loop... */
	if (inReplay(details.pos))
		return;

	/* Ignore soft errors under estimation mode.
	 * These are going to be reported later on anyway */
	if (!details.shouldHalt && inEstimationMode())
		return;

	/* If this is an invalid access, change the RF of the offending
	 * event to BOTTOM, so that we do not try to get its value.
	 * Don't bother updating the views */
	auto *errLab = details.pos.isBottom() ? nullptr : g.getEventLabel(details.pos);
	if (errLab && isInvalidAccessError(details.type) && llvm::isa<ReadLabel>(errLab))
		llvm::dyn_cast<ReadLabel>(errLab)->setRf(nullptr);

	/* Print a basic error message and the graph.
	 * We have to save the interpreter state as replaying will
	 * destroy the current execution stack */

	MIRI_LOG() << "TODO GENMC: reportError: get EE to save its state and replay an execution\n";
	// auto iState = getEE()->saveState();

	// getEE()->replayExecutionBefore(*getReplayView());

	/* Refetch ERRLAB in case it's a block label and was replaced during replay.
	 * (This may happen when replaying assume reads.) */
	errLab = details.pos.isBottom() ? nullptr : g.getEventLabel(details.pos);

	llvm::raw_string_ostream out(result.message);

	out << (isHardError(details.type) ? "Error: " : "Warning: ") << details.type << "!\n";
	if (errLab)
		out << "Event " << errLab->getPos() << " ";
	if (details.racyLab != nullptr)
		out << "conflicts with event " << details.racyLab->getPos() << " ";
	out << "in graph:\n";
	printGraph(true, out);

	/* Print error trace leading up to the violating event(s) */
	if (errLab && getConf()->printErrorTrace) {
		printTraceBefore(errLab, out);
		if (details.racyLab != nullptr)
			printTraceBefore(details.racyLab, out);
	}

	/* Print the specific error message */
	if (!details.msg.empty())
		out << details.msg << "\n";

	/* Dump the graph into a file (DOT format) */
	if (!getConf()->dotFile.empty())
		dotPrintToFile(getConf()->dotFile, errLab, details.racyLab,
			       getConf()->dotPrintOnlyClientEvents);

	// getEE()->restoreState(std::move(iState));
	MIRI_LOG() << "TODO GENMC: reportError: get EE to restore its state (unimplemented)\n";

	if (details.shouldHalt)
		halt(details.type);
}

bool GenMCDriver::reportWarningOnce(Event pos, VerificationError wcode,
				    const EventLabel *racyLab /* = nullptr */)
{
	MIRI_LOG() << "GenMCDriver::reportWarningOnce\n";
	/* Helper function to determine whether the warning should be treated as an error */
	auto shouldUpgradeWarning = [&](auto &wcode) {
		if (wcode != VerificationError::VE_WWRace)
			return std::make_pair(false, ""s);
		if (!getConf()->symmetryReduction && !getConf()->ipr)
			return std::make_pair(false, ""s);

		auto &g = getExec().getGraph();
		auto *lab = g.getEventLabel(pos);
		auto upgrade =
			(getConf()->symmetryReduction &&
			 std::ranges::any_of(
				 g.thr_ids(),
				 [&](auto tid) {
					 return g.getFirstThreadLabel(tid)->getSymmPredTid() != -1;
				 })) ||
			(getConf()->ipr && std::ranges::any_of(samelocs(g, lab), [&](auto &oLab) {
				 auto *rLab = llvm::dyn_cast<ReadLabel>(&oLab);
				 return rLab && rLab->getAnnot();
			 }));
		auto [cause, cli] =
			getConf()->ipr
				? std::make_pair("in-place revisiting (IPR)"s, "-disable-ipr"s)
				: std::make_pair("symmetry reduction (SR)"s, "-disable-sr"s);
		auto msg = "Unordered writes do not constitute a bug per se, though they often "
			   "indicate faulty design.\n" +
			   (upgrade ? ("This warning is treated as an error due to " + cause +
				       ".\n"
				       "You can use " +
				       cli + " to disable these features."s)
				    : ""s);
		return std::make_pair(upgrade, msg);
	};

	/* If the warning has been seen before, only report it if it's an error */
	auto [upgradeWarning, msg] = shouldUpgradeWarning(wcode);
	auto &knownWarnings = getResult().warnings;
	if (upgradeWarning || knownWarnings.count(wcode) == 0) {
		reportError({pos, wcode, msg, racyLab, upgradeWarning});
	}
	if (knownWarnings.count(wcode) == 0)
		knownWarnings.insert(wcode);
	if (wcode == VerificationError::VE_WWRace)
		getExec().getGraph().getWriteLabel(pos)->setAttr(WriteAttr::WWRacy);
	return upgradeWarning;
}

bool GenMCDriver::checkBarrierWellFormedness(BIncFaiWriteLabel *sLab)
{
	if (getConf()->disableBAM)
		return true;

	/* Find the latest round completion (or init, if none exists) */
	auto &g = getExec().getGraph();
	auto lastIt = std::ranges::find_if(g.rco(sLab->getAddr()), [sLab](const auto &lab) {
		return &lab != sLab &&
		       ((llvm::isa<BIncFaiWriteLabel>(lab) &&
			 isLastInBarrierRound(llvm::dyn_cast<BIncFaiWriteLabel>(&lab))) ||
			lab.isNotAtomic());
	});

	/* Check whether the last barrier completion is hb;po;po-before SLAB */
	auto ok = getConsChecker().getHbView(g.po_imm_pred(sLab)).contains(lastIt->getPos());
	if (!ok) {
		reportError({sLab->getPos(), VerificationError::VE_BarrierWellFormedness,
			     "Execution not barrier-well-formed!\n"});
	}
	return ok;
}

bool GenMCDriver::tryOptimizeBarrierRevisits(BIncFaiWriteLabel *sLab,
					     std::vector<ReadLabel *> &loads)
{
	if (getConf()->disableBAM)
		return false;

	if (!checkBarrierWellFormedness(sLab) || !isLastInBarrierRound(sLab))
		return true;

	/* Find reads to revisit. `loads` is disregarded because it
	 * might not contain some valid revisits (e.g., discarded due to
	 * maximality-related optimizations) */
	auto &g = *sLab->getParent();
	auto *wLab = llvm::dyn_cast<ReadLabel>(g.po_imm_pred(sLab))->getRf();
	BUG_ON(!wLab);
	std::vector<ReadLabel *> toRevisit;

	while (llvm::isa<BIncFaiWriteLabel>(wLab) &&
	       !isLastInBarrierRound(llvm::dyn_cast<BIncFaiWriteLabel>(wLab))) {
		auto *nLab = g.po_imm_succ(wLab);
		if (nLab) {
			BUG_ON(!llvm::isa<BWaitReadLabel>(nLab));
			toRevisit.push_back(llvm::cast<ReadLabel>(nLab));
		}
		wLab = llvm::dyn_cast<ReadLabel>(g.po_imm_pred(wLab))->getRf();
	}

	/* Finally, revisit in place */
	for (auto *lab : toRevisit) {
		BUG_ON(!llvm::isa<BWaitReadLabel>(lab));
		revisitInPlace(*constructBackwardRevisit(lab, sLab));
	}
	return true;
}

void GenMCDriver::tryOptimizeIPRs(const WriteLabel *sLab, std::vector<ReadLabel *> &loads)
{
	if (!getConf()->ipr)
		return;

	auto &g = getExec().getGraph();

	std::vector<ReadLabel *> toIPR;
	loads.erase(std::remove_if(loads.begin(), loads.end(),
				   [&](auto *rLab) {
					   /* Treatment of blocked CASes is different */
					   auto blocked =
						   !llvm::isa<CasReadLabel>(rLab) &&
						   rLab->getAnnot() &&
						   !rLab->valueMakesAssumeSucceed(
							   rLab->getAccessValue(rLab->getAccess()));
					   if (blocked)
						   toIPR.push_back(rLab);
					   return blocked;
				   }),
		    loads.end());

	for (auto *rLab : toIPR)
		revisitInPlace(*constructBackwardRevisit(rLab, sLab));

	/* We also have to filter out some regular revisits */
	auto *confLab = findPendingRMW(sLab);
	if (!confLab)
		return;

	loads.erase(std::remove_if(loads.begin(), loads.end(),
				   [&](auto *rLab) {
					   auto *rfLab = rLab->getRf();
					   return rLab->getAnnot() && // must be like that
						  rfLab->getStamp() > rLab->getStamp() &&
						  !getPrefixView(sLab).contains(rfLab->getPos());
				   }),
		    loads.end());
}

bool GenMCDriver::removeCASReadIfBlocks(const ReadLabel *rLab, const EventLabel *sLab)
{
	auto &g = getExec().getGraph();
	/* This only affects annotated CASes */
	if (!rLab->getAnnot() || !llvm::isa<CasReadLabel>(rLab) ||
	    (!getConf()->ipr && !llvm::isa<LockCasReadLabel>(rLab)))
		return false;
	/* Skip if bounding is enabled or the access is uninitialized */
	if (isUninitializedAccess(rLab->getAddr(), sLab->getPos()) || getConf()->bound.has_value())
		return false;

	/* If the CAS blocks, block thread altogether */
	auto val = sLab->getAccessValue(rLab->getAccess());
	if (rLab->valueMakesAssumeSucceed(val))
		return false;

	blockThread(g, ReadOptBlockLabel::create(rLab->getPos(), rLab->getAddr()));
	return true;
}

void GenMCDriver::checkReconsiderReadOpts(const WriteLabel *sLab)
{
	auto &g = getExec().getGraph();
	for (auto i = 0U; i < g.getNumThreads(); i++) {
		auto *bLab = llvm::dyn_cast_or_null<ReadOptBlockLabel>(g.getLastThreadLabel(i));
		if (!bLab || bLab->getAddr() != sLab->getAddr())
			continue;
		unblockThread(g, bLab->getPos());
	}
}

void GenMCDriver::optimizeUnconfirmedRevisits(const WriteLabel *sLab,
					      std::vector<ReadLabel *> &loads)
{
	if (!getConf()->confirmation)
		return;

	auto &g = getExec().getGraph();

	/* If there is already a write with the same value, report a possible ABA */
	auto valid = std::count_if(
		g.co_begin(sLab->getAddr()), g.co_end(sLab->getAddr()), [&](auto &wLab) {
			return wLab.getPos() != sLab->getPos() && wLab.getVal() == sLab->getVal();
		});
	if (sLab->getAddr().isStatic() &&
	    g.getInitLabel()->getAccessValue(sLab->getAccess()) == sLab->getVal())
		++valid;
	WARN_ON_ONCE(valid > 0 && std::ranges::count_if(
					  loads, [](auto *lab) { return lab->isConfirming(); }),
		     "confirmation-aba-found",
		     "Possible ABA pattern! Consider running without -confirmation.\n");

	/* Do not bother with revisits that will be unconfirmed/lead to ABAs */
	loads.erase(std::remove_if(loads.begin(), loads.end(),
				   [&](auto *lab) {
					   if (!lab->isConfirming())
						   return false;

					   const EventLabel *scLab = nullptr;
					   auto *pLab = findMatchingSpeculativeRead(lab, scLab);
					   ERROR_ON(!pLab, "Confirming CAS annotation error! "
							   "Does a speculative read precede the "
							   "confirming operation?\n");

					   return !scLab;
				   }),
		    loads.end());
}

bool GenMCDriver::tryOptimizeRevisits(WriteLabel *sLab, std::vector<ReadLabel *> &loads)
{
	auto &g = getExec().getGraph();

	/* BAM */
	if (!getConf()->disableBAM) {
		if (auto *faiLab = llvm::dyn_cast<BIncFaiWriteLabel>(sLab)) {
			if (tryOptimizeBarrierRevisits(faiLab, loads))
				return true;
		}
	}

	/* IPR */
	tryOptimizeIPRs(sLab, loads);

	/* Confirmation: Do not bother with revisits that will lead to unconfirmed reads */
	if (getConf()->confirmation)
		optimizeUnconfirmedRevisits(sLab, loads);
	return false;
}

void GenMCDriver::revisitInPlace(const BackwardRevisit &br)
{
	BUG_ON(getConf()->bound.has_value());

	auto &g = getExec().getGraph();
	auto *rLab = g.getReadLabel(br.getPos());
	auto *sLab = g.getWriteLabel(br.getRev());

	BUG_ON(!llvm::isa<ReadLabel>(rLab));
	if (g.po_imm_succ(rLab))
		g.removeLast(rLab->getThread());
	rLab->setRf(sLab);
	rLab->setAddedMax(true); // always true for atomicity violations

	/* CASes shouldn't be handled via IPRs */
	BUG_ON(rLab->valueMakesRMWSucceed(rLab->getReturnValue()));

	GENMC_DEBUG(LOG(VerbosityLevel::Debug1) << "--- In-place revisiting " << rLab->getPos()
						<< " <-- " << sLab->getPos() << "\n"
						<< getExec().getGraph(););
}

void updatePredsWithPrefixView(const ExecutionGraph &g, VectorClock &preds,
			       const VectorClock &pporf)
{
	/* In addition to taking (preds U pporf), make sure pporf includes rfis */
	preds.update(pporf);

	if (!dynamic_cast<const DepExecutionGraph *>(&g))
		return;
	auto &predsD = *llvm::dyn_cast<DepView>(&preds);
	for (auto i = 0u; i < pporf.size(); i++) {
		for (auto j = 1; j <= pporf.getMax(i); j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
				if (preds.contains(rLab->getPos()) &&
				    !preds.contains(rLab->getRf())) {
					if (rLab->getRf()->getThread() == rLab->getThread())
						predsD.removeHole(rLab->getRf()->getPos());
				}
			}
			auto *wLab = llvm::dyn_cast<WriteLabel>(lab);
			if (wLab && wLab->isRMW() && pporf.contains(lab->getPos().prev()))
				predsD.removeHole(lab->getPos());
		}
	}
	return;
}

std::unique_ptr<VectorClock> GenMCDriver::getRevisitView(const ReadLabel *rLab,
							 const WriteLabel *sLab) const
{
	auto &g = getExec().getGraph();
	auto preds = g.getPredsView(rLab->getPos());

	updatePredsWithPrefixView(g, *preds, getPrefixView(sLab));
	return preds;
}

auto GenMCDriver::constructBackwardRevisit(const ReadLabel *rLab, const WriteLabel *sLab) const
	-> std::unique_ptr<BackwardRevisit>
{
	return std::make_unique<BackwardRevisit>(rLab, sLab, getRevisitView(rLab, sLab));
}

bool isFixedHoleInView(const ExecutionGraph &g, const EventLabel *lab, const DepView &v)
{
	if (auto *wLabB = llvm::dyn_cast<WriteLabel>(lab))
		return std::any_of(wLabB->readers_begin(), wLabB->readers_end(),
				   [&v](auto &oLab) { return v.contains(oLab.getPos()); });

	auto *rLabB = llvm::dyn_cast<ReadLabel>(lab);
	if (!rLabB)
		return false;

	/* If prefix has same address load, we must read from the same write */
	for (auto i = 0u; i < v.size(); i++) {
		for (auto j = 0u; j <= v.getMax(i); j++) {
			if (!v.contains(Event(i, j)))
				continue;
			if (auto *mLab = g.getReadLabel(Event(i, j)))
				if (mLab->getAddr() == rLabB->getAddr() &&
				    mLab->getRf() == rLabB->getRf())
					return true;
		}
	}

	if (rLabB->isRMW()) {
		auto *wLabB = g.getWriteLabel(rLabB->getPos().next());
		return std::any_of(wLabB->readers_begin(), wLabB->readers_end(),
				   [&v](auto &oLab) { return v.contains(oLab.getPos()); });
	}
	return false;
}

bool GenMCDriver::prefixContainsSameLoc(const BackwardRevisit &r, const EventLabel *lab) const
{
	if (!getConf()->isDepTrackingModel)
		return false;

	/* Some holes need to be treated specially. However, it is _wrong_ to keep
	 * porf views around. What we should do instead is simply check whether
	 * an event is "part" of WLAB's pporf view (even if it is not contained in it). */
	auto &g = getExec().getGraph();
	auto &v = *llvm::dyn_cast<DepView>(&getPrefixView(g.getEventLabel(r.getRev())));
	if (lab->getIndex() <= v.getMax(lab->getThread()) && isFixedHoleInView(g, lab, v))
		return true;
	return false;
}

bool GenMCDriver::isCoBeforeSavedPrefix(const BackwardRevisit &r, const EventLabel *lab)
{
	auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab);
	if (!mLab)
		return false;

	auto &g = getExec().getGraph();
	auto &v = r.getViewNoRel();
	auto rLab = llvm::dyn_cast<ReadLabel>(mLab);
	auto wLab = g.getWriteLabel(rLab ? rLab->getRf()->getPos() : mLab->getPos());

	auto succIt = wLab ? g.co_succ_begin(wLab) : g.co_begin(mLab->getAddr());
	auto succE = wLab ? g.co_succ_end(wLab) : g.co_end(mLab->getAddr());
	return any_of(succIt, succE, [&](auto &sLab) {
		/* Exclude the write that revisits from the prefix */
		return sLab.getPos() != r.getRev() && v->contains(sLab.getPos()) &&
		       (!getConf()->isDepTrackingModel ||
			mLab->getIndex() > getPrefixView(&sLab).getMax(mLab->getThread()));
	});
}

bool GenMCDriver::coherenceSuccRemainInGraph(const BackwardRevisit &r)
{
	auto &g = getExec().getGraph();
	auto *wLab = g.getWriteLabel(r.getRev());
	if (wLab->isRMW())
		return true;

	auto succIt = g.co_succ_begin(wLab);
	auto succE = g.co_succ_end(wLab);
	if (succIt == succE)
		return true;

	return r.getViewNoRel()->contains(succIt->getPos());
}

bool wasAddedMaximally(const EventLabel *lab)
{
	if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(lab))
		return mLab->wasAddedMax();
	if (auto *oLab = llvm::dyn_cast<OptionalLabel>(lab))
		return !oLab->isExpanded();
	return true;
}

bool GenMCDriver::isMaximalExtension(const BackwardRevisit &r)
{
	/* Only revisit when the write's direct successor (if any) remains in the graph;
	 * revisits should not only differ on the write's placement */
	if (!coherenceSuccRemainInGraph(r))
		return false;

	auto &g = getExec().getGraph();
	auto &v = r.getViewNoRel();

	for (const auto &lab : g.labels()) {
		/* Exclude events unaffected by the revisit */
		if ((lab.getPos() != r.getPos() && v->contains(lab.getPos())) ||
		    prefixContainsSameLoc(r, &lab))
			continue;

		if (!lab.isRevisitable())
			return false;
		if (!wasAddedMaximally(&lab))
			return false;
		if (isCoBeforeSavedPrefix(r, &lab))
			return false;
	}
	return true;
}

bool GenMCDriver::revisitModifiesGraph(const BackwardRevisit &r) const
{
	const auto &g = getExec().getGraph();
	const auto &v = r.getViewNoRel();
	for (auto i = 0U; i < g.getNumThreads(); i++) {
		if (v->getMax(i) + 1 != (long)g.getThreadSize(i) &&
		    !llvm::isa<TerminatorLabel>(g.getEventLabel(Event(i, v->getMax(i) + 1))))
			return true;
		if (!getConf()->isDepTrackingModel)
			continue;
		for (auto j = 0U; j < g.getThreadSize(i); j++) {
			auto *lab = g.getEventLabel(Event(i, j));
			if (!v->contains(lab->getPos()) && !llvm::isa<EmptyLabel>(lab) &&
			    !llvm::isa<TerminatorLabel>(lab))
				return true;
		}
	}
	return false;
}

std::unique_ptr<ExecutionGraph> GenMCDriver::copyGraph(const BackwardRevisit *br,
						       VectorClock *v) const
{
	auto &g = getExec().getGraph();

	/* Adjust the view that will be used for copying */
	auto &prefix = getPrefixView(g.getEventLabel(br->getRev()));
	auto og = g.getCopyUpTo(*v);

	/** Ensure the prefix of the write will not be revisitable.
	 * This is also used to check whether a write has revisited,
	 * and appropriately prevent some revisits */
	auto *revLab = og->getReadLabel(br->getPos());

	for (auto &lab : og->labels()) {
		if (prefix.contains(lab.getPos()))
			lab.setRevisitStatus(false);
	}
	return og;
}

void GenMCDriver::calcRevisits(WriteLabel *sLab)
{
	MIRI_LOG() << "GenMCDriver::calcRevisits: TODO GENMC: error handling\n";
	auto &g = getExec().getGraph();
	auto loads = getRevisitableApproximation(sLab);

	GENMC_DEBUG(LOG(VerbosityLevel::Debug3) << "Revisitable: " << format(loads) << "\n";);
	if (tryOptimizeRevisits(sLab, loads))
		return;

	/* If operating in estimation mode, don't actually revisit */
	if (inEstimationMode()) {
		getExec().getChoiceMap().update(loads, sLab);
		return;
	}

	GENMC_DEBUG(LOG(VerbosityLevel::Debug3)
			    << "Revisitable (optimized): " << format(loads) << "\n";);
	for (auto *rLab : loads) {
		auto br = constructBackwardRevisit(rLab, sLab);
		if (!isMaximalExtension(*br))
			break;

		getExec().getWorkqueue().add(std::move(br));
	}
}

auto GenMCDriver::completeRevisitedRMW(const ReadLabel *rLab) -> WriteLabel *
{
	auto wLab = createRMWWriteLabel(getExec().getGraph(), rLab);
	if (!wLab)
		return nullptr;

	auto *lab = llvm::dyn_cast<WriteLabel>(addLabelToGraph(std::move(wLab)));
	BUG_ON(!rLab->getRf());
	lab->addCo(rLab->getRf());
	return lab;
}

bool GenMCDriver::revisitWrite(const WriteForwardRevisit &wi)
{
	auto &g = getExec().getGraph();
	auto *wLab = g.getWriteLabel(wi.getPos());
	BUG_ON(!wLab);

	wLab->moveCo(g.getEventLabel(wi.getPred()));
	wLab->setAddedMax(false);
	calcRevisits(wLab);
	return !violatesAtomicity(wLab);
}

bool GenMCDriver::revisitOptional(const OptionalForwardRevisit &oi)
{
	auto &g = getExec().getGraph();
	auto *oLab = llvm::dyn_cast<OptionalLabel>(g.getEventLabel(oi.getPos()));

	--result.exploredBlocked;
	BUG_ON(!oLab);
	oLab->setExpandable(false);
	oLab->setExpanded(true);
	return true;
}

bool GenMCDriver::revisitRead(const Revisit &ri)
{
	BUG_ON(!llvm::isa<ReadRevisit>(&ri));

	/* We are dealing with a read: change its reads-from and also check
	 * whether a part of an RMW should be added */
	auto &g = getExec().getGraph();
	auto *rLab = g.getReadLabel(ri.getPos());
	auto *revLab = g.getEventLabel(llvm::dyn_cast<ReadRevisit>(&ri)->getRev());

	rLab->setRf(revLab);
	auto *fri = llvm::dyn_cast<ReadForwardRevisit>(&ri);
	rLab->setAddedMax(fri ? fri->isMaximal() : revLab == g.co_max(rLab->getAddr()));

	GENMC_DEBUG(LOG(VerbosityLevel::Debug1)
			    << "--- " << (llvm::isa<BackwardRevisit>(ri) ? "Backward" : "Forward")
			    << " revisiting " << ri.getPos() << " <-- " << revLab->getPos() << "\n"
			    << getExec().getGraph(););

	/*  Try to remove the read from the execution */
	if (removeCASReadIfBlocks(rLab, revLab))
		return true;

	if (checkInitializedMem(rLab) != VerificationError::VE_OK)
		return false;

	/* If the revisited label became an RMW, add the store part and revisit */
	if (auto *sLab = completeRevisitedRMW(rLab)) {
		calcRevisits(sLab);
		return !violatesAtomicity(sLab);
	}

	/* Blocked barrier or blocked lock: block thread */
	if (llvm::isa<BWaitReadLabel>(rLab) &&
	    !readsBarrierUnblockingValue(llvm::cast<BWaitReadLabel>(rLab)))
		blockThread(g, BarrierBlockLabel::create(rLab->getPos().next()));
	/* Blocked lock: block thread */
	if (llvm::isa<LockCasReadLabel>(rLab))
		blockThread(g, LockNotAcqBlockLabel::create(rLab->getPos().next()));
	return true;
}

bool GenMCDriver::forwardRevisit(const ForwardRevisit &fr)
{
	auto &g = getExec().getGraph();
	auto *lab = g.getEventLabel(fr.getPos());
	if (auto *mi = llvm::dyn_cast<WriteForwardRevisit>(&fr))
		return revisitWrite(*mi);
	if (auto *oi = llvm::dyn_cast<OptionalForwardRevisit>(&fr))
		return revisitOptional(*oi);
	if (auto *rr = llvm::dyn_cast<RerunForwardRevisit>(&fr))
		return true;
	auto *ri = llvm::dyn_cast<ReadForwardRevisit>(&fr);
	BUG_ON(!ri);
	return revisitRead(*ri);
}

bool GenMCDriver::backwardRevisit(const BackwardRevisit &br)
{
	auto &g = getExec().getGraph();

	/* Recalculate the view because some B labels might have been
	 * removed */
	auto v = getRevisitView(g.getReadLabel(br.getPos()), g.getWriteLabel(br.getRev()));

	auto og = copyGraph(&br, &*v);
	auto cmap = ChoiceMap(getExec().getChoiceMap());
	cmap.cut(*v);
	auto alloctor = SAddrAllocator(getExec().getAllocator());
	alloctor.restrict(createAllocView(*og));

	pushExecution(
		{std::move(og), LocalQueueT(), std::move(cmap), std::move(alloctor), br.getPos()});

	repairDanglingReads(getExec().getGraph());
	auto ok = revisitRead(br);
	BUG_ON(!ok);

	/* If there are idle workers in the thread pool,
	 * try submitting the job instead */
	auto *tp = getThreadPool();
	if (tp && tp->getRemainingTasks() < 8 * tp->size()) {
		if (isRevisitValid(br))
			tp->submit(extractState());
		return false;
	}
	return true;
}

bool GenMCDriver::restrictAndRevisit(const WorkList::ItemT &item)
{
	MIRI_LOG() << "GenMCDriver::restrictAndRevisit\n";
	/* First, appropriately restrict the worklist and the graph */
	auto &g = getExec().getGraph();
	auto *br = llvm::dyn_cast<BackwardRevisit>(&*item);
	auto stamp = g.getEventLabel(br ? br->getRev() : item->getPos())->getStamp();
	getExec().restrict(stamp);

	getExec().getLastAdded() = item->getPos();
	if (auto *fr = llvm::dyn_cast<ForwardRevisit>(&*item))
		return forwardRevisit(*fr);
	if (auto *br = llvm::dyn_cast<BackwardRevisit>(&*item)) {
		return backwardRevisit(*br);
	}
	BUG();
	return false;
}

bool GenMCDriver::handleHelpingCas(std::unique_ptr<HelpingCasLabel> hLab)
{
	BUG_ON(!getConf()->helper);

	if (isExecutionDrivenByGraph(&*hLab))
		return true;

	/* Ensure that the helped CAS exists */
	auto &g = getExec().getGraph();
	auto *lab = llvm::dyn_cast<HelpingCasLabel>(addLabelToGraph(std::move(hLab)));
	if (!checkHelpingCasCondition(lab)) {
		blockThread(g, HelpedCASBlockLabel::create(lab->getPos()));
		return false;
	}
	return true;
}

bool GenMCDriver::handleOptional(std::unique_ptr<OptionalLabel> lab)
{
	auto &g = getExec().getGraph();

	if (isExecutionDrivenByGraph(&*lab))
		return llvm::dyn_cast<OptionalLabel>(g.getEventLabel(lab->getPos()))->isExpanded();

	if (std::any_of(g.label_begin(), g.label_end(), [&](auto &lab) {
		    auto *oLab = llvm::dyn_cast<OptionalLabel>(&lab);
		    return oLab && !oLab->isExpandable();
	    }))
		lab->setExpandable(false);

	auto *oLab = llvm::dyn_cast<OptionalLabel>(addLabelToGraph(std::move(lab)));

	if (!inEstimationMode() && oLab->isExpandable())
		getExec().getWorkqueue().add(
			std::make_unique<OptionalForwardRevisit>(oLab->getPos()));
	return false; /* should not be expanded yet */
}

void GenMCDriver::handleSpinStart(std::unique_ptr<SpinStartLabel> lab)
{
	auto &g = getExec().getGraph();

	/* If it has not been added to the graph, do so */
	if (isExecutionDrivenByGraph(&*lab))
		return;

	auto *stLab = addLabelToGraph(std::move(lab));

	/* Check whether we can detect some spinloop dynamically */
	auto stpreds = po_preds(g, stLab);
	auto lbLabIt = std::ranges::find_if(
		stpreds, [](auto &lab) { return llvm::isa<LoopBeginLabel>(lab); });

	/* If we did not find a loop-begin, this a manual instrumentation(?); report to user
	 */
	ERROR_ON(lbLabIt == stpreds.end(), "No loop-beginning found!\n");

	auto *lbLab = &*lbLabIt;
	auto pLabIt = std::ranges::find_if(stpreds, [lbLab](auto &lab) {
		return llvm::isa<SpinStartLabel>(&lab) && lab.getIndex() > lbLab->getIndex();
	});
	if (pLabIt == stpreds.end())
		return;

	auto *pLab = &*pLabIt;
	for (auto i = pLab->getIndex() + 1; i < stLab->getIndex(); i++) {
		auto *wLab =
			llvm::dyn_cast<WriteLabel>(g.getEventLabel(Event(stLab->getThread(), i)));
		if (wLab && wLab->isEffectful() && wLab->isObservable())
			return; /* found event w/ side-effects */
	}
	/* Spinloop detected */
	blockThreadTryMoot(SpinloopBlockLabel::create(stLab->getPos()));
}

bool GenMCDriver::areFaiZNEConstraintsSat(const FaiZNESpinEndLabel *lab)
{
	auto &g = getExec().getGraph();

	/* Check that there are no other side-effects since the previous iteration.
	 * We don't have to look for a BEGIN label since ZNE labels are always
	 * preceded by a spin-start */
	auto preds = po_preds(g, lab);
	auto ssLabIt = std::ranges::find_if(
		preds, [](auto &lab) { return llvm::isa<SpinStartLabel>(&lab); });
	BUG_ON(ssLabIt == preds.end());
	auto *ssLab = &*ssLabIt;
	for (auto i = ssLab->getIndex() + 1; i < lab->getIndex(); ++i) {
		auto *oLab = g.getEventLabel(Event(ssLab->getThread(), i));
		if (llvm::isa<WriteLabel>(oLab) && !llvm::isa<FaiWriteLabel>(oLab))
			return false;
	}

	auto wLabIt = std::ranges::find_if(
		preds, [](auto &lab) { return llvm::isa<FaiWriteLabel>(&lab); });
	BUG_ON(wLabIt == preds.end());

	/* All stores in the RMW chain need to be read from at most 1 read,
	 * and there need to be no other stores that are not hb-before lab */
	auto *wLab = llvm::dyn_cast<FaiWriteLabel>(&*wLabIt);
	for (auto &lab : g.labels()) {
		if (auto *mLab = llvm::dyn_cast<MemAccessLabel>(&lab)) {
			if (mLab->getAddr() == wLab->getAddr() && !llvm::isa<FaiReadLabel>(mLab) &&
			    !llvm::isa<FaiWriteLabel>(mLab) &&
			    !getConsChecker().getHbView(wLab).contains(mLab->getPos()))
				return false;
		}
	}
	return true;
}

void GenMCDriver::handleFaiZNESpinEnd(std::unique_ptr<FaiZNESpinEndLabel> lab)
{
	auto &g = getExec().getGraph();

	/* If we are actually replaying this one, it is not a spin loop*/
	if (isExecutionDrivenByGraph(&*lab))
		return;

	auto *zLab = llvm::dyn_cast<FaiZNESpinEndLabel>(addLabelToGraph(std::move(lab)));
	if (areFaiZNEConstraintsSat(zLab))
		blockThread(g, FaiZNEBlockLabel::create(zLab->getPos())); /* no moot desired */
}

void GenMCDriver::handleLockZNESpinEnd(std::unique_ptr<LockZNESpinEndLabel> lab)
{
	if (isExecutionDrivenByGraph(&*lab))
		return;

	auto *zLab = addLabelToGraph(std::move(lab));
	blockThreadTryMoot(LockZNEBlockLabel::create(zLab->getPos()));
}

void GenMCDriver::handleDummy(std::unique_ptr<EventLabel> lab)
{
	if (!isExecutionDrivenByGraph(&*lab))
		addLabelToGraph(std::move(lab));
}

/************************************************************
 ** Printing facilities
 ***********************************************************/

static void executeMDPrint(const EventLabel *lab, const std::pair<int, std::string> &locAndFile,
			   std::string inputFile, llvm::raw_ostream &os = llvm::outs())
{
	std::string errPath = locAndFile.second;
	Parser::stripSlashes(errPath);
	Parser::stripSlashes(inputFile);

	os << " ";
	if (errPath != inputFile)
		os << errPath << ":";
	else
		os << "L.";
	os << locAndFile.first;
}

/* Returns true if the corresponding LOC should be printed for this label type */
bool shouldPrintLOC(const EventLabel *lab)
{
	/* Begin/End labels don't have a corresponding LOC */
	if (llvm::isa<ThreadStartLabel>(lab) || llvm::isa<ThreadFinishLabel>(lab))
		return false;

	/* Similarly for allocations that don't come from malloc() */
	if (const auto *mLab = llvm::dyn_cast<MallocLabel>(lab))
		return mLab->getAllocAddr().isHeap() && !mLab->getAllocAddr().isInternal();

	return true;
}

std::string GenMCDriver::getVarName(const SAddr &addr) const
{
	if (addr.isStatic())
		return "[Global ???]";
	// BUG(); // TODO GENMC: pass required info (getStaticName)
	// return getEE()->getStaticName(addr);

	const auto &g = getExec().getGraph();
	const auto *aLab = findAllocatingLabel(g, addr);

	if (!aLab)
		return "???";
	if (aLab->getNameInfo())
		return aLab->getName() +
		       aLab->getNameInfo()->getNameAtOffset(addr - aLab->getAllocAddr());
	return "";
}

void GenMCDriver::printGraph(bool printMetadata /* false */,
			     llvm::raw_ostream &s /* = llvm::dbgs() */)
{
	if (!hasExec()) {
		s << "(NO GRAPH TO PRINT)\n";
		return;
	}
	auto &g = getExec().getGraph();
	LabelPrinter printer([this](const SAddr &saddr) { return getVarName(saddr); },
			     [this](const ReadLabel &lab) {
				     return lab.getRf() ? lab.getAccessValue(lab.getAccess())
							: SVal();
			     });

	/* Print the graph */
	// BUG(); // TODO GENMC: pass required thread info
	MIRI_LOG() << "TODO GENMC: printGraph: get EE to print its graph (unimplemented)\n";

	for (auto i = 0u; i < g.getNumThreads(); i++) {
		// auto &thr = EE->getThrById(i);
		// s << thr;
		// s << "(SKIP: thread)"; // TODO GENMC
		s << "Thread " << i;
		if (getConf()->symmetryReduction) {
			if (auto *bLab = g.getFirstThreadLabel(i)) {
				auto symm = bLab->getSymmPredTid();
				if (symm != -1)
					s << " symmetric with " << symm;
			}
		}
		s << ":\n";
		for (auto &lab : g.po(i)) {
			if (llvm::isa<ThreadStartLabel>(lab))
				continue;
			s << "\t" << printer.toString(lab);
			GENMC_DEBUG(if (getConf()->printStamps) s << " @ " << lab.getStamp(););
			if (printMetadata)
				s << "(SKIP: thr.MetaData)";
			// if (printMetadata && thr.prefixLOC[lab.getIndex()].first &&
			// 		    shouldPrintLOC(&lab)) {
			// 	executeMDPrint(&lab, thr.prefixLOC[lab.getIndex()],
			// 				       getConf()->inputFile, s);
			// }
			s << "\n";
		}
	}

	/* MO: Print coherence information */
	auto header = false;
	for (auto locIt = g.loc_begin(), locE = g.loc_end(); locIt != locE; ++locIt) {
		/* Skip empty and single-store locations */
		if (g.hasLocMoreThanOneStore(locIt->first)) {
			if (!header) {
				s << "Coherence:\n";
				header = true;
			}
			auto *wLab = &*g.co_begin(locIt->first);
			s << getVarName(wLab->getAddr()) << ": [ ";
			for (const auto &w : g.co(locIt->first))
				s << w << " ";
			s << "]\n";
		}
	}
	s << "\n";
}

void GenMCDriver::dotPrintToFile(const std::string &filename, const EventLabel *errLab,
				 const EventLabel *confLab, bool printObservation)
{
	auto &g = getExec().getGraph();
	auto *EE = getEE();
	std::ofstream fout(filename);
	llvm::raw_os_ostream ss(fout);
	DotPrinter printer([this](const SAddr &saddr) { return getVarName(saddr); },
			   [this](const ReadLabel &lab) {
				   return lab.getRf() ? lab.getAccessValue(lab.getAccess())
						      : SVal();
			   });

	unique_ptr<VectorClock> before;
	if (errLab)
		before = getPrefixView(errLab).clone();
	else
		before = g.getViewFromStamp(g.getMaxStamp());

	if (confLab)
		before->update(getPrefixView(confLab));

	/* Create a directed graph */
	ss << "strict digraph {\n";
	/* Specify node shape */
	ss << "node [shape=plaintext]\n";
	/* Left-justify labels for clusters */
	ss << "labeljust=l\n";
	/* Draw straight lines */
	ss << "splines=false\n";

	/* Print all nodes with each thread represented by a cluster */
	for (auto i = 0u; i < before->size(); i++) {
		BUG(); // TODO GENMC: pass required thread info

		// bool inMethod = false;
		// auto &thr = EE->getThrById(i);
		// ss << "subgraph cluster_" << thr.id << "{\n";
		// ss << "\tlabel=\"" << thr.threadFun->getName().str() << "()\"\n";
		// ss << "\ttooltip=\"thread #" << thr.id << "\"\n";
		// for (auto j = 1; j <= before->getMax(i); j++) {
		// 	auto *lab = g.getEventLabel(Event(i, j));

		// if (printObservation) {
		// 	if (llvm::isa<MethodBeginLabel>(lab))
		// 		inMethod = true;
		// 	else if (llvm::isa<MethodEndLabel>(lab))
		// 		inMethod = false;
		// 	else if (inMethod)
		// 		continue;
		// }
		// 	ss << "\t\"" << lab->getPos() << "\" [label=<";

		// 	/* First, print the graph label for this node */
		// 	ss << printer.toString(*lab);

		// 	/* And then, print the corresponding line number */
		// 	if (thr.prefixLOC[j].first && shouldPrintLOC(lab)) {
		// 		ss << " <FONT COLOR=\"gray\">";
		// 		executeMDPrint(lab, thr.prefixLOC[j], getConf()->inputFile, ss);
		// 		ss << "</FONT>";
		// 	}
		// 	ss << ">";

		// 	if (errLab && lab->getPos() == errLab->getPos())
		// 		ss << ", style=filled, fillcolor=yellow";
		//	if (confLab && lab->getPos() == confLab->getPos())
		// 		ss << ", style=filled, fillcolor=yellow";

		// 	ss << ", tooltip=\"" << lab->getPos() << "\"";
		// 	ss << "]\n";
		// }
		// ss << "}\n";
	}

	/* Print relations between events (po U rf) */
	for (auto i = 0u; i < before->size(); i++) {
		BUG(); // TODO GENMC: pass required thread info
		       // bool inMethod = false;
		       // auto &thr = EE->getThrById(i);
		       // EventLabel const *lastLab = nullptr;
		       // for (auto j = 0; j <= before->getMax(i); j++) {
		       // 	auto *lab = g.getEventLabel(Event(i, j));

		// 	if (printObservation) {
		// 		if (llvm::isa<MethodBeginLabel>(lab))
		// 			inMethod = true;
		// 		else if (llvm::isa<MethodEndLabel>(lab))
		// 			inMethod = false;
		// 		else if (inMethod)
		// 			continue;
		// 	}

		// 	/* Print a po-edge, but skip dummy start events for
		// 	 * all threads except for the first one */
		// 	if (j < before->getMax(i) && !llvm::isa<ThreadStartLabel>(lab))
		// 		ss << "\"" << lab->getPos() << "\" -> \"" << lab->getPos().next()
		// 		   << "\"\n";
		// 	if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab)) {
		// 		/* Do not print RFs from INIT, BOTTOM, and same thread */
		// 		if (llvm::dyn_cast_or_null<WriteLabel>(rLab) &&
		// 		    rLab->getRf()->getThread() != lab->getThread()) {
		// 			ss << "\"" << rLab->getRf() << "\" -> \"" << rLab->getPos()
		// 			   << "\"[color=green, constraint=false]\n";
		// 		}
		// 	}
		// 	if (auto *bLab = llvm::dyn_cast<ThreadStartLabel>(lab)) {
		// 		if (thr.id == 0)
		// 			continue;
		// 		ss << "\"" << bLab->getParentCreate() << "\" -> \""
		// 		   << bLab->getPos().next() << "\"[color=blue, constraint=false]\n";
		// 	}
		// 	if (auto *jLab = llvm::dyn_cast<ThreadJoinLabel>(lab))
		// 		ss << "\"" << g.getLastThreadLabel(jLab->getChildId())->getPos()
		// 		   << "\" -> \"" << jLab->getPos()
		// 		   << "\"[color=blue, constraint=false]\n";
		// }
	}

	ss << "}\n";
}

void GenMCDriver::recPrintTraceBefore(const Event &e, View &a,
				      llvm::raw_ostream &ss /* llvm::outs() */)
{
	BUG(); // TODO GENMC: reenable
	       // const auto &g = getExec().getGraph();

	// if (a.contains(e))
	// 	return;

	// auto ai = a.getMax(e.thread);
	// a.setMax(e);
	// auto &thr = getEE()->getThrById(e.thread);
	// for (int i = ai; i <= e.index; i++) {
	// 	const EventLabel *lab = g.getEventLabel(Event(e.thread, i));
	// 	if (auto *rLab = llvm::dyn_cast<ReadLabel>(lab))
	// 		if (rLab->getRf())
	// 			recPrintTraceBefore(rLab->getRf()->getPos(), a, ss);
	// 	if (auto *jLab = llvm::dyn_cast<ThreadJoinLabel>(lab))
	// 		recPrintTraceBefore(g.getLastThreadLabel(jLab->getChildId())->getPos(), a,
	// 				    ss);
	// 	if (auto *bLab = llvm::dyn_cast<ThreadStartLabel>(lab))
	// 		if (!bLab->getCreateId().isInitializer())
	// 			recPrintTraceBefore(bLab->getCreateId(), a, ss);

	// 	/* Do not print the line if it is an RMW write, since it will be
	// 	 * the same as the previous one */
	// 	if (llvm::isa<CasWriteLabel>(lab) || llvm::isa<FaiWriteLabel>(lab))
	// 		continue;
	// 	/* Similarly for a Wna just after the creation of a thread
	// 	 * (it is the store of the PID) */
	// 	if (i > 0 && llvm::isa<ThreadCreateLabel>(g.po_imm_pred(lab)))
	// 		continue;
	// 	Parser::parseInstFromMData(thr.prefixLOC[i], thr.threadFun->getName().str(), ss);
	// }
	// return;
}

void GenMCDriver::printTraceBefore(const EventLabel *lab, llvm::raw_ostream &s /* = llvm::dbgs() */)
{
	s << "Trace to " << lab->getPos() << ":\n";

	/* Linearize (po U rf) and print trace */
	View view;
	recPrintTraceBefore(lab->getPos(), view, s);
}
