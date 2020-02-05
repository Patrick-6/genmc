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

#ifndef __ADJ_LIST_HPP__
#define __ADJ_LIST_HPP__

#include <llvm/ADT/BitVector.h>
#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct pair_hash {
	inline std::size_t
	operator()(const std::pair<unsigned int, unsigned int> &v) const {
		return v.first * 31 + v.second;
	}
};

template <class T, class Hash = std::hash<T> >
class AdjList {

protected:
	/* Simple aliases to easily defer what function arguments represent */
	using NodeId = unsigned int;
	using Timestamp = unsigned int;

	/* Node status during DFS exploration */
	enum class NodeStatus { unseen, entered, left };

public:
	AdjList() {}
	AdjList(const std::vector<T> &es) {
		for (auto i = 0u; i < es.size(); i++)
			addNode(elems[i]);
	}

	/* Iterator typedefs */
	using iterator = typename std::vector<T>::iterator;
	using const_iterator = typename std::vector<T>::const_iterator;
	using reverse_iterator = typename std::vector<T>::reverse_iterator;
	using const_revserse_iteratr = typename std::vector<T>::const_reverse_iterator;

	/* Iterators -- they iterate over the nodes of the graph */
	iterator begin() { return elems.begin(); };
	iterator end() { return elems.end(); };
	const_iterator begin() const { return elems.begin(); };
	const_iterator end() const { return elems.end(); };

	/* Returns the elements (nodes) of the graph */
	const std::vector<T> &getElems() const { return elems; }

	/* Adds a node to the graph */
	void addNode(T a);

	/* Adds a new edge to the graph */
	void addEdge(T a, T b);

	/* Returns the in-degree of each element */
	std::vector<unsigned int> getInDegrees() const;

	/* Performs a DFS exploration */
	template<typename FC, typename FN, typename FE, typename FEND>
	void dfs(FC&& onCycle, FN&& onNeighbor, FE&& atExplored, FEND&& atEnd);

	/* Returns a topological sorting of the graph */
	template<typename F>
	std::vector<T> topoSort(F&& onSort);

	/* Runs prop on all topological sortings */
	template<typename F>
	bool allTopoSort(F&& prop) const;

	template<typename F>
	bool combineAllTopoSort(const std::vector<AdjList<T, Hash> *> &toCombine, F&& prop);

	void transClosure();

	/* Returns true if the respective edge exists */
	inline bool operator()(const T a, const T b) const {
		return edges.count(std::make_pair(getId(a), getId(b)));
	}
	inline bool operator()(NodeId a, NodeId b) const {
		return edges.count(std::make_pair(a, b));
	}

	template<typename U, typename Z>
	friend llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const AdjList<U, Z> &l);

private:
	/* Helper for addEdge() that adds nodes with known IDs */
	void addEdge(NodeId a, NodeId b);

	/* Helper for dfs() */
	template<typename FC, typename FN, typename FE>
	void dfsUtil(NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
		     std::vector<NodeId> &p, std::vector<Timestamp> &d,
		     std::vector<Timestamp> &f, FC&& onCycle,
		     FN&& onNeighbor, FE&& atExplored);

	template<typename F>
	bool allTopoSortUtil(std::vector<T> &current, std::vector<bool> visited,
			     std::vector<int> &inDegree, F&& prop, bool &found) const;

	template<typename F>
	bool combineAllTopoSortUtil(unsigned int index, std::vector<std::vector<T>> &current,
				    bool &found, const std::vector<AdjList<T, Hash> *> &toCombine,
				    F&& prop);

	/* The node elements.
	 * Must be in 1-1 correspondence with the lists below */
	std::vector<T> elems;

	/* The successor/predecessor list for each node */
	std::vector<std::vector<NodeId> > nodeSucc;
	std::vector<std::vector<NodeId> > nodePred;

	/* Map that maintains the ID of each element */
	std::unordered_map<T, NodeId, Hash> ids;

	/* Keep all the edges in a map for quick access */
	std::unordered_set<std::pair<NodeId, NodeId>, pair_hash> edges;

	/* Maintain transitive closure info */
	bool calculatedTransC = false;
	std::vector<llvm::BitVector> transC;
};

#include "AdjList.tcc"

#endif /* __ADJ_LIST_HPP__ */
