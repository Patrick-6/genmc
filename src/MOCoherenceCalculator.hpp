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

#ifndef __MO_COHERENCE_CALCULATOR_HPP__
#define __MO_COHERENCE_CALCULATOR_HPP__

#include "CoherenceCalculator.hpp"
#include "Error.hpp"
#include <unordered_map>

/*******************************************************************************
 **                    MOCoherenceCalculator Class
 ******************************************************************************/

/*
 * An implementation of the CoherenceCalculator API, that tracks coherence
 * by recording the modification order. Should be used along with the "mo"
 * version of the driver.
 */
class MOCoherenceCalculator : public CoherenceCalculator {

public:

	/* Constructor */
	MOCoherenceCalculator(ExecutionGraph &g, bool ooo)
		: CoherenceCalculator(CC_ModificationOrder, g, ooo) {}

	/* Returns the range of all the possible (i.e., not violating coherence
	 * places a store can be inserted without inserting it */
	virtual std::pair<int, int>
	getPossiblePlacings(const llvm::GenericValue *addr,
			    Event store, bool isRMW) override;

	/* Tracks a store by inserting it into the appropriate offset */
	virtual void
	addStoreToLoc(const llvm::GenericValue *addr, Event store, int offset) override;

	/* Returns a list of stores to a particular memory location */
	const std::vector<Event>&
	getStoresToLoc(const llvm::GenericValue *addr) override;

	/* Returns all the stores for which if "read" reads-from, coherence
	 * is not violated */
	std::vector<Event>
	getCoherentStores(const llvm::GenericValue *addr, Event read) override;

	/* Stops tracking all stores not included in "preds" in the graph */
	void removeStoresAfter(VectorClock &preds) override;

	/* Changes the offset of "store" to "newOffset" */
	void changeStoreOffset(const llvm::GenericValue *addr,
			       Event store, int newOffset);

	static bool classof(const CoherenceCalculator *cohTracker) {
		return cohTracker->getKind() == CC_ModificationOrder;
	}

private:

	/* Returns the offset for a particular store */
	int getStoreOffset(const llvm::GenericValue *addr, Event e) const;

	/* Returns the index of the first store that is _not_ (rf?;hb)-before
	 * the event "read". If no such stores exist (i.e., all stores are
	 * concurrent in models that do not support out-of-order execution),
	 * it returns 0. */
	int splitLocMOBefore(const llvm::GenericValue *addr, Event read);

	/* Returns the index of the first store that is hb-after "read",
	 * or the next index of the first store that is read by a read that
	 * is hb-after "read". Returns 0 if the latter condition holds for
	 * the initializer event, and the number of the stores in "addr"
	 * if the conditions do not hold for any store in that location. */
	int splitLocMOAfterHb(const llvm::GenericValue *addr, const Event read);

	/* Similar to splitLocMOAfterHb(), but used for calculating possible MO
	 * placings. This means it does not take into account reads-from the
	 * initializer, and also returns the index (as opposed to index+1) of
	 * the first store that is hb-after "s" or is read by a read that is
	 * hb-after "s" */
	int splitLocMOAfter(const llvm::GenericValue *addr, const Event s);


	typedef std::unordered_map<const llvm::GenericValue *,
				   std::vector<Event> > ModifOrder;
	ModifOrder mo_;
};

#endif /* __COHERENCE_CALCULATOR_HPP__ */
