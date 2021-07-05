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

#include "ThreadPool.hpp"

std::unique_ptr<llvm::Module> cloneModule(const std::unique_ptr<llvm::Module> &mod,
					  const std::unique_ptr<llvm::LLVMContext> &ctx)
{
	// Otherwise, round trip the module to a stream and then back
	// into the new context.  This approach allows for duplication
	// and optimization to proceed in parallel for different
	// modules.
	std::string         str;
	llvm::raw_string_ostream  stream(str);
	llvm::WriteBitcodeToFile(*mod, stream);

	llvm::StringRef                   ref(stream.str());
	std::unique_ptr<llvm::MemoryBuffer>
		buf(llvm::MemoryBuffer::getMemBuffer(ref));
	return std::move(llvm::parseBitcodeFile(buf->getMemBufferRef(), *ctx).get());
}

ThreadPinner::ThreadPinner(unsigned int n) : numTasks(n)
{
	if (hwloc_topology_init(&topology) < 0 || hwloc_topology_load(topology) < 0)
		ERROR("Error during topology initialization\n");
	cpusets.resize(n);

	hwloc_obj_t root = hwloc_get_root_obj(topology);
	int result = hwloc_distrib(topology, &root, 1, cpusets.data(),
				   numTasks, std::numeric_limits<int>::max(), 0u);
	if (result)
		ERROR("Error during topology distribution\n");

	/* Minimize migration costs */
	for (int i = 0; i < numTasks; i++)
		hwloc_bitmap_singlify(cpusets[i]);
}

void ThreadPinner::pin(std::thread &t, unsigned int cpu)
{
	if (hwloc_set_thread_cpubind(topology, t.native_handle(), cpusets[cpu], 0) == -1)
		ERROR("Error during thread pinning\n");
}

void ThreadPool::addWorker(unsigned int i, std::unique_ptr<GenMCDriver> d)
{
	std::packaged_task<GenMCDriver::Result(unsigned int, std::unique_ptr<GenMCDriver> driver)> t([this](unsigned int i, std::unique_ptr<GenMCDriver> driver){
		     while (true) {
			     // std::this_thread::sleep_for(std::chrono::milliseconds((i == 0) ? 1000 : 0));
			     auto state = popTask();

			     /* If the state is empty, we should halt */
			     if (!state)
				     break;

			     /* Prepare the driver and start the exploration */
			     driver->setSharedState(std::move(state));
			     // llvm::dbgs() << "WORKER " << i << " picked up a task\n";
			     activeThreads.fetch_add(1, std::memory_order_relaxed);
			     driver->run();
			     decRemainingTasks();
			     activeThreads.fetch_sub(1, std::memory_order_relaxed);

			     if (remainingTasks.load(std::memory_order_relaxed) == 0) {
				     halt();
				     break;
			     }
		     }
		     /* Do some printing here and maybe move driver to the result */
		     return driver->getResult();
	});

	results.push_back(std::move(t.get_future()));

	workers.push_back(std::thread(std::move(t), i, std::move(d)));
	pinner.pin(workers.back(), i);
	return;
}

void ThreadPool::submit(std::unique_ptr<TaskT> t)
{
	incRemainingTasks();
	queue.push(std::move(t));
	return;
}

std::unique_ptr<ThreadPool::TaskT> ThreadPool::tryPopPoolQueue()
{
	return queue.tryPop();
}

std::unique_ptr<ThreadPool::TaskT> ThreadPool::tryStealOtherQueue()
{
	/* TODO: Implement work-stealing */
	return nullptr;
}

std::unique_ptr<ThreadPool::TaskT> ThreadPool::popTask()
{
	do {
		if (auto t = tryPopPoolQueue())
			return t;
		else if (auto t = tryStealOtherQueue())
			return t;
		else
			std::this_thread::yield();
	} while (!shouldHalt.load(std::memory_order_relaxed));
	return nullptr;
}

std::vector<std::future<GenMCDriver::Result>> ThreadPool::waitForTasks()
{
	while (remainingTasks.load(std::memory_order_relaxed) > 0 // ||
	       // !shouldHalt.load(std::memory_order_relaxed)
		)
		std::this_thread::yield();

	return std::move(results);
}
