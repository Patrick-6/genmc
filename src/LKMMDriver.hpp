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

#ifndef __LKMM_DRIVER_HPP__
#define __LKMM_DRIVER_HPP__

#include "GenMCDriver.hpp"

class LKMMDriver : public GenMCDriver {

public:

	LKMMDriver(std::unique_ptr<Config> conf, std::unique_ptr<llvm::Module> mod,
		   std::vector<Library> &granted, std::vector<Library> &toVerify,
		   clock_t start);

	/* Label-creation methods  */
	std::unique_ptr<ReadLabel>
	createReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			const llvm::GenericValue *ptr, const llvm::Type *typ,
			Event rf) override;
	std::unique_ptr<FaiReadLabel>
	createFaiReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			   const llvm::GenericValue *ptr, const llvm::Type *typ,
			   Event rf, llvm::AtomicRMWInst::BinOp op,
			   const llvm::GenericValue &opValue,
			   FaiReadLabel::FaiType ft) override;
	std::unique_ptr<CasReadLabel>
	createCasReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			   const llvm::GenericValue *ptr, const llvm::Type *typ,
			   Event rf, const llvm::GenericValue &expected,
			   const llvm::GenericValue &swap,
			   bool isLock = false) override;
	std::unique_ptr<DskReadLabel>
	createDskReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			   const llvm::GenericValue *ptr, const llvm::Type *typ,
			   Event rf) override;
	std::unique_ptr<LibReadLabel>
	createLibReadLabel(int tid, int index, llvm::AtomicOrdering ord,
			   const llvm::GenericValue *ptr, const llvm::Type *typ,
			   Event rf, std::string functionName)  override;
	std::unique_ptr<WriteLabel>
	createStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
			 const llvm::GenericValue *ptr, const llvm::Type *typ,
			 const llvm::GenericValue &val,
			 bool isUnlock = false) override;
	std::unique_ptr<FaiWriteLabel>
	createFaiStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
			    const llvm::GenericValue *ptr, const llvm::Type *typ,
			    const llvm::GenericValue &val) override;
	std::unique_ptr<CasWriteLabel>
	createCasStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
			    const llvm::GenericValue *ptr, const llvm::Type *typ,
			    const llvm::GenericValue &val,
			    bool isLock = false) override;
	std::unique_ptr<LibWriteLabel>
	createLibStoreLabel(int tid, int index, llvm::AtomicOrdering ord,
			    const llvm::GenericValue *ptr, const llvm::Type *typ,
			    llvm::GenericValue &val, std::string functionName,
			    bool isInit) override;
	std::unique_ptr<DskWriteLabel>
	createDskWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
			    const llvm::GenericValue *ptr, const llvm::Type *typ,
			    const llvm::GenericValue &val, void *mapping) override;

	std::unique_ptr<DskMdWriteLabel>
	createDskMdWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
			      const llvm::GenericValue *ptr, const llvm::Type *typ,
			      const llvm::GenericValue &val, void *mapping,
			      std::pair<void *, void *> ordDataRange) override;

	std::unique_ptr<DskDirWriteLabel>
	createDskDirWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
			       const llvm::GenericValue *ptr, const llvm::Type *typ,
			       const llvm::GenericValue &val, void *mapping) override;

	std::unique_ptr<DskJnlWriteLabel>
	createDskJnlWriteLabel(int tid, int index, llvm::AtomicOrdering ord,
			       const llvm::GenericValue *ptr, const llvm::Type *typ,
			       const llvm::GenericValue &val, void *mapping, void *transInode) override;
	std::unique_ptr<FenceLabel>
	createFenceLabel(int tid, int index, llvm::AtomicOrdering ord) override;
	std::unique_ptr<SmpFenceLabelLKMM>
	createSmpFenceLabelLKMM(int tid, int index, const char *lkmmType) override;
	std::unique_ptr<MallocLabel>
	createMallocLabel(int tid, int index, const void *addr,
			  unsigned int size, Storage s, AddressSpace spc) override;
	std::unique_ptr<DskOpenLabel>
	createDskOpenLabel(int tid, int index, const char *fileName,
			   const llvm::GenericValue &fd) override;
	std::unique_ptr<DskFsyncLabel>
	createDskFsyncLabel(int tid, int index, const void *inode,
			    unsigned int size) override;
	std::unique_ptr<DskSyncLabel>
	createDskSyncLabel(int tid, int index) override;
	std::unique_ptr<DskPbarrierLabel>
	createDskPbarrierLabel(int tid, int index) override;
	std::unique_ptr<FreeLabel>
	createFreeLabel(int tid, int index, const void *addr) override;
	std::unique_ptr<ThreadCreateLabel>
	createTCreateLabel(int tid, int index, int cid) override;
	std::unique_ptr<ThreadJoinLabel>
	createTJoinLabel(int tid, int index, int cid) override;
	std::unique_ptr<ThreadStartLabel>
	createStartLabel(int tid, int index, Event tc, int symm = -1) override;
	std::unique_ptr<ThreadFinishLabel>
	createFinishLabel(int tid, int index) override;
	std::unique_ptr<LockLabelLAPOR>
	createLockLabelLAPOR(int tid, int index, const llvm::GenericValue *addr) override;
	std::unique_ptr<UnlockLabelLAPOR>
	createUnlockLabelLAPOR(int tid, int index, const llvm::GenericValue *addr) override;
	std::unique_ptr<RCULockLabelLKMM>
	createRCULockLabelLKMM(int tid, int index) override;
	std::unique_ptr<RCUUnlockLabelLKMM>
	createRCUUnlockLabelLKMM(int tid, int index) override;
	std::unique_ptr<RCUSyncLabelLKMM>
	createRCUSyncLabelLKMM(int tid, int index) override;

	Event findDataRaceForMemAccess(const MemAccessLabel *mLab) override;
	std::vector<Event> getStoresToLoc(const llvm::GenericValue *addr) override;
	std::vector<Event> getRevisitLoads(const WriteLabel *lab) override;
	void changeRf(Event read, Event store) override;
	bool updateJoin(Event join, Event childLast) override;
	void initConsCalculation() override;

private:

	View calcBasicHbView(Event e) const;
	DepView calcFenceView(const MemAccessLabel *lab) const;
	DepView calcPPoView(EventLabel *lab); /* not const */
	void updateRelView(DepView &pporf, EventLabel *lab);
	void updateReadViewsFromRf(DepView &pporf, View &hb, ReadLabel *rLab);
	void calcBasicReadViews(ReadLabel *lab);
	void calcBasicWriteViews(WriteLabel *lab);
	void calcWriteMsgView(WriteLabel *lab);
	void calcRMWWriteMsgView(WriteLabel *lab);
	void updateRmbFenceView(DepView &pporf, SmpFenceLabelLKMM *lab);
	void updateWmbFenceView(DepView &pporf, SmpFenceLabelLKMM *lab);
	void updateMbFenceView(DepView &pporf, SmpFenceLabelLKMM *fLab);
	void calcBasicFenceViews(SmpFenceLabelLKMM *lab);
	void calcFenceRelRfPoBefore(Event last, View &v);

	bool areInPotentialRace(const MemAccessLabel *labA, const MemAccessLabel *labB);
	std::vector<Event> findPotentialRacesForNewLoad(const ReadLabel *rLab);
	std::vector<Event> findPotentialRacesForNewStore(const WriteLabel *wLab);


	bool isNoRetRead(const EventLabel *lab);
	bool isNoRetRead(Event r);
	bool isAcqPoOrPoRelBefore(Event a, Event b);
	bool isFenceBefore(Event a, Event b);

	std::vector<Event> getOverwrites(const MemAccessLabel *lab);
	std::vector<Event> getMarkedWritePreds(const EventLabel *lab);
	std::vector<Event> getMarkedWriteSuccs(const EventLabel *lab);
	std::vector<Event> getMarkedReadPreds(const EventLabel *lab);
	std::vector<Event> getMarkedReadSuccs(const EventLabel *lab);
	std::vector<Event> getStrongFenceSuccs(Event e);

	bool isVisConnected(Event a, Event b);
	bool isVisBefore(Event a, Event b);

	bool isWWVisBefore(const MemAccessLabel *labA, const MemAccessLabel *labB);
	bool isWRVisBefore(const MemAccessLabel *labA, const MemAccessLabel *labB);
	bool isRWXbBefore(const MemAccessLabel *labA, const MemAccessLabel *labB);
	bool isWRXbBefore(const MemAccessLabel *labA, const MemAccessLabel *labB);

	bool isValidWWRace(const WriteLabel *labA, const WriteLabel *labB);
	bool isValidWRRace(const WriteLabel *labA, const ReadLabel *labB);
	bool isValidRWRace(const ReadLabel *labA, const WriteLabel *labB);
	bool isValidRace(Event a, Event b);
};

#endif /* __LKMM_DRIVER_HPP__ */
