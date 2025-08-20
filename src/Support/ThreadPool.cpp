/*
 * GenMC -- Generic Model Checking.
 *
 * This project is dual-licensed under the Apache License 2.0 and the MIT License.
 * You may choose to use, distribute, or modify this software under either license.
 *
 * Apache License 2.0:
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * MIT License:
 *     https://opensource.org/licenses/MIT
 */

#include "ThreadPool.hpp"
#include "Verification/VerificationResult.hpp"

void ThreadPool::addWorker(unsigned int i, std::unique_ptr<GenMCDriver> driver,
			   std::unique_ptr<llvm::Interpreter> EE, TFunT threadFun)
{
	using ThreadT = std::packaged_task<VerificationResult(
		unsigned int, std::unique_ptr<GenMCDriver> driver,
		std::unique_ptr<llvm::Interpreter> EE, TFunT threadFun)>;

	ThreadT thread([this](unsigned int /*i*/, std::unique_ptr<GenMCDriver> driver,
			      std::unique_ptr<llvm::Interpreter> EE, TFunT threadFun) {
		while (true) {
			auto taskUP = popTask();

			/* If the state is empty, nothing left to do */
			if (!taskUP)
				break;

			/* Prepare the driver and start the exploration */
			driver->initFromState(std::move(taskUP), EE->getCallbacks());
			threadFun(&*driver, &*EE);

			/* If that was the last task, notify everyone */
			std::lock_guard<std::mutex> lock(stateMtx_);
			if (decRemainingTasks() == 0) {
				stateCV_.notify_all();
				break;
			}
		}
		return std::move(driver->getResult());
	});

	results_.push_back(std::move(thread.get_future()));

	workers_.emplace_back(std::move(thread), i, std::move(driver), std::move(EE),
			      std::move(threadFun));
	pinner_.pin(workers_.back(), i);
}

void ThreadPool::submit(ThreadPool::TaskT t)
{
	std::lock_guard<std::mutex> lock(stateMtx_);
	incRemainingTasks();
	queue_.push(std::move(t));
	stateCV_.notify_one();
}

auto ThreadPool::tryPopPoolQueue() -> ThreadPool::TaskT { return queue_.tryPop(); }

auto ThreadPool::tryStealOtherQueue() -> ThreadPool::TaskT
{
	/* TODO: Implement work-stealing */
	return nullptr;
}

auto ThreadPool::popTask() -> ThreadPool::TaskT
{
	while (true) {
		if (auto t = tryPopPoolQueue())
			return t;
		if (auto t = tryStealOtherQueue())
			return t;

		std::unique_lock<std::mutex> lock(stateMtx_);
		if (shouldHalt() || getRemainingTasks() == 0)
			return nullptr;
		stateCV_.wait(lock);
	}
	return nullptr;
}

auto ThreadPool::waitForTasks() -> std::vector<std::future<VerificationResult>>
{
	while (!shouldHalt() && getRemainingTasks() > 0)
		std::this_thread::yield();

	return std::move(results_);
}
