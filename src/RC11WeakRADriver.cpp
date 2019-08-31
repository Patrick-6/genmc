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

#include "RC11WeakRADriver.hpp"

/************************************************************
 ** WEAK RA DRIVER -- UNIMPLEMENTED
 ***********************************************************/


std::unique_ptr<ReadLabel>
RC11WeakRADriver::createReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				  const llvm::GenericValue *ptr, const llvm::Type *typ,
				  Event rf)
{
}

std::unique_ptr<FaiReadLabel>
RC11WeakRADriver::createFaiReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				     const llvm::GenericValue *ptr, const llvm::Type *typ,
				     Event rf, llvm::AtomicRMWInst::BinOp op,
				     llvm::GenericValue &&opValue)
{
}

std::unique_ptr<CasReadLabel>
RC11WeakRADriver::createCasReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				     const llvm::GenericValue *ptr, const llvm::Type *typ,
				     Event rf, const llvm::GenericValue &expected,
				     const llvm::GenericValue &swap,
				     bool isLock)
{
}

std::unique_ptr<LibReadLabel>
RC11WeakRADriver::createLibReadLabel(int tid, int index, llvm::AtomicOrdering ord,
				     const llvm::GenericValue *ptr, const llvm::Type *typ,
				     Event rf, std::string functionName)
{
}

std::unique_ptr<WriteLabel>
RC11WeakRADriver::createStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				   const llvm::GenericValue *ptr, const llvm::Type *typ,
				   const llvm::GenericValue &val, int offsetMO,
				   bool isUnlock)
{
}

std::unique_ptr<FaiWriteLabel>
RC11WeakRADriver::createFaiStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				      const llvm::GenericValue *ptr, const llvm::Type *typ,
				      const llvm::GenericValue &val, int offsetMO)
{
}

std::unique_ptr<CasWriteLabel>
RC11WeakRADriver::createCasStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				      const llvm::GenericValue *ptr, const llvm::Type *typ,
				      const llvm::GenericValue &val, int offsetMO,
				      bool isLock)
{
}

std::unique_ptr<LibWriteLabel>
RC11WeakRADriver::createLibStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
				      const llvm::GenericValue *ptr, const llvm::Type *typ,
				      llvm::GenericValue &val, int offsetMO,
				      std::string functionName, bool isInit)
{
}

std::unique_ptr<FenceLabel>
RC11WeakRADriver::createFenceLabel(int tid, int index, llvm::AtomicOrdering ord)
{
}


std::unique_ptr<MallocLabel>
RC11WeakRADriver::createMallocLabel(int tid, int index, const void *addr,
				    unsigned int size, bool isLocal)
{
}

std::unique_ptr<FreeLabel>
RC11WeakRADriver::createFreeLabel(int tid, int index, const void *addr,
				  unsigned int size)
{
}

std::unique_ptr<ThreadCreateLabel>
RC11WeakRADriver::createTCreateLabel(int tid, int index, int cid)
{
}

std::unique_ptr<ThreadJoinLabel>
RC11WeakRADriver::createTJoinLabel(int tid, int index, int cid)
{
}

std::unique_ptr<ThreadStartLabel>
RC11WeakRADriver::createStartLabel(int tid, int index, Event tc)
{
}

std::unique_ptr<ThreadFinishLabel>
RC11WeakRADriver::createFinishLabel(int tid, int index)
{
}

std::vector<Event> RC11WeakRADriver::findOverwrittenBoundary(const llvm::GenericValue *addr,
							     int thread)
{
	const auto &g = getGraph();
	auto &before = g.getHbBefore(g.getLastThreadEvent(thread));
	std::vector<Event> boundary;

	if (before.empty())
		return boundary;

	for (auto &e : g.getModOrderAtLoc(addr))
		if (g.isWriteRfBefore(before, e))
			boundary.push_back(e.prev());
	return boundary;
}

std::vector<Event> RC11WeakRADriver::getStoresToLoc(const llvm::GenericValue *addr)
{
	const auto &g = getGraph();
	auto overwritten = findOverwrittenBoundary(addr, getEE()->getCurThr().id);
	std::vector<Event> stores;

	if (overwritten.empty()) {
		auto &locMO = g.getModOrderAtLoc(addr);
		stores.push_back(Event::getInitializer());
		stores.insert(stores.end(), locMO.begin(), locMO.end());
		return stores;
	}

	auto before = g.getHbRfBefore(overwritten);
	for (auto i = 0u; i < g.getNumThreads(); i++) {
		for (auto j = before[i] + 1u; j < g.getThreadSize(i); j++) {
			const EventLabel *lab = g.getEventLabel(Event(i, j));
			if (auto *wLab = llvm::dyn_cast<WriteLabel>(lab))
				if (wLab->getAddr() == addr)
					stores.push_back(wLab->getPos());
		}
	}
	return stores;
}

std::pair<int, int> RC11WeakRADriver::getPossibleMOPlaces(const llvm::GenericValue *addr, bool isRMW)
{
	BUG();
}

std::vector<Event> RC11WeakRADriver::getRevisitLoads(const WriteLabel *lab)
{
	BUG();
}

std::pair<std::vector<std::unique_ptr<EventLabel> >,
	  std::vector<std::pair<Event, Event> > >
RC11WeakRADriver::getPrefixToSaveNotBefore(const WriteLabel *sLab, const ReadLabel *rLab)
{
	BUG();
}

void RC11WeakRADriver::changeRf(Event read, Event store)
{
	BUG();
}

void RC11WeakRADriver::resetJoin(Event join)
{
	BUG();
}

bool RC11WeakRADriver::updateJoin(Event join, Event childLast)
{
	BUG();
}

bool RC11WeakRADriver::checkPscAcyclicity(CheckPSCType t)
{
	WARN("Unimplemented!\n");
	abort();
}

bool RC11WeakRADriver::isExecutionValid()
{
	WARN("Unimplemented!\n");
	abort();
}
