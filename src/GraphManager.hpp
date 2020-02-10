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

#ifndef __GRAPH_MANAGER_HPP__
#define __GRAPH_MANAGER_HPP__

#include "AdjList.hpp"
#include "Calculator.hpp"
#include "ExecutionGraph.hpp"
#include <unordered_map>

class CoherenceCalculator;
class LBCalculatorLAPOR;
class PSCCalculator;

/*******************************************************************************
 **                           GraphManager Class
 ******************************************************************************/

/*
 * A simple class that manages an execution graph and relations on said graph.
 * Instances of this class should be created via the GraphBuilder class.
 * It is instantiated differently depending on the memory model, and is
 * largely responsible for performing the consistency checks. The relations
 * needed to be calculated may entail the initiation of one or more fixpoints.
 */
class GraphManager {

public:
	enum class RelationId { hb, co, lb, psc, ar };

	/* Constructors */
	GraphManager() = delete;
	GraphManager(std::unique_ptr<ExecutionGraph> g);

	/* Returns the execution graph object */
	ExecutionGraph &getGraph() { return *graph; };
	ExecutionGraph &getGraph() const { return *graph; };

	/* Adds the specified calculator to the list */
	void addCalculator(std::unique_ptr<Calculator> cc, RelationId r,
			   bool perLoc, bool partial = false);

	/* Returns the list of the calculators */
	const std::vector<const Calculator *> getCalcs() const;

	/* Returns the list of the partial calculators */
	const std::vector<const Calculator *> getPartialCalcs() const;

	/* Returns a reference to the specified relation matrix */
	Calculator::GlobalRelation& getGlobalRelation(RelationId id);
	Calculator::PerLocRelation& getPerLocRelation(RelationId id);

	/* Returns a reference to the cached version of the
	 * specified relation matrix */
	Calculator::GlobalRelation& getCachedGlobalRelation(RelationId id);
	Calculator::PerLocRelation& getCachedPerLocRelation(RelationId id);

	/* Caches all calculated relations. If "copy" is true then a
	 * copy of each relation is cached */
	void cacheRelations(bool copy = true);

	/* Restores all relations to their most recently cached versions.
	 * If "move" is true then the cache is cleared as well */
	void restoreCached(bool move = false);

	/* Returns a pointer to the specified relation's calculator */
	Calculator *getCalculator(RelationId id);

	/* Commonly queried calculator getters */
	CoherenceCalculator *getCoherenceCalculator();
	CoherenceCalculator *getCoherenceCalculator() const;
	LBCalculatorLAPOR *getLbCalculatorLAPOR();

	std::vector<Event> getLbOrderingLAPOR();

	void doInits(bool fullCalc = false);

	/* Performs a step of all the specified calculations. Takes as
	 * a parameter whether a full calculation needs to be performed */
	Calculator::CalculationResult doCalcs(bool fullCalc = false);

	/* Event addition methods */
	const ReadLabel *addReadLabelToGraph(std::unique_ptr<ReadLabel> lab,
					     Event rf);
	const WriteLabel *addWriteLabelToGraph(std::unique_ptr<WriteLabel> lab,
					       unsigned int offsetMO);
	const WriteLabel *addWriteLabelToGraph(std::unique_ptr<WriteLabel> lab,
					       Event pred);
	const LockLabelLAPOR *addLockLabelToGraphLAPOR(std::unique_ptr<LockLabelLAPOR> lab);
	const EventLabel *addOtherLabelToGraph(std::unique_ptr<EventLabel> lab);

	/* Modification order methods */
	void trackCoherenceAtLoc(const llvm::GenericValue *addr);
	const std::vector<Event>& getStoresToLoc(const llvm::GenericValue *addr) const;
	const std::vector<Event>& getStoresToLoc(const llvm::GenericValue *addr);
	std::vector<Event> getCoherentStores(const llvm::GenericValue *addr,
					     Event pos);
	std::pair<int, int> getCoherentPlacings(const llvm::GenericValue *addr,
						Event pos, bool isRMW);
	std::vector<Event> getCoherentRevisits(const WriteLabel *wLab);

	void changeStoreOffset(const llvm::GenericValue *addr,
			       Event s, int newOffset);

	std::vector<std::pair<Event, Event> >
	saveCoherenceStatus(const std::vector<std::unique_ptr<EventLabel> > &prefix,
			    const ReadLabel *rLab) const;

	void cutToLabel(const EventLabel *rLab);

	void restoreStorePrefix(const ReadLabel *rLab,
				std::vector<std::unique_ptr<EventLabel> > &storePrefix,
				std::vector<std::pair<Event, Event> > &moPlacings);

private:

	/* The actual graph object */
	std::unique_ptr<ExecutionGraph> graph;

	/* A list of all the calculations that need to be performed
	 * when checking for full consistency*/
	std::vector<std::unique_ptr<Calculator> > consistencyCalculators;

	/* The indices of all the calculations that need to be performed
	 * at each step of the algorithm (partial consistency check) */
	std::vector<int> partialConsCalculators;

	/* The relation matrices (and caches) maintained in the manager */
	std::vector<Calculator::GlobalRelation> globalRelations;
	std::vector<Calculator::GlobalRelation> globalRelationsCache;
	std::vector<Calculator::PerLocRelation> perLocRelations;
	std::vector<Calculator::PerLocRelation> perLocRelationsCache;

	/* Keeps track of calculator indices */
	std::unordered_map<RelationId, unsigned int> calculatorIndex;

	/* Keeps track of relation indices. Note that an index might
	 * refer to either globalRelations or perLocRelations */
	std::unordered_map<RelationId, unsigned int> relationIndex;
};

#include "CoherenceCalculator.hpp"
#include "LBCalculatorLAPOR.hpp"
// #include "PSCCalculator.hpp"

#endif /* __GRAPH_MANAGER_HPP__ */
