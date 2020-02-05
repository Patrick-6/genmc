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

#include "Error.hpp"

template<typename T, typename H>
void AdjList<T, H>::addNode(T a)
{
	auto id = elems.size();

	ids[a] = id;
	elems.push_back(a);
	nodeSucc.push_back({});
	nodePred.push_back({});

	calculatedTransC = false;
	transC.push_back(llvm::BitVector());
	return;
}

template<typename T, typename H>
void AdjList<T, H>::addEdge(NodeId a, NodeId b)
{
	/* If edge already exists, nothing to do */
	if ((*this)(a, b))
		return;

	edges.insert(std::make_pair(a, b));
	nodeSucc[a].push_back(b);
	nodePred[b].push_back(a);
	calculatedTransC = false;
	return;
}

template<typename T, typename H>
void AdjList<T, H>::addEdge(T a, T b)
{
	BUG_ON(ids.count(a) == 0 || ids.count(b) == 0);
	addEdge(ids[a], ids[b]);
	return;
}

template<typename T, typename H>
std::vector<unsigned int> AdjList<T, H>::getInDegrees() const
{
	std::vector<unsigned int> inDegree(elems.size(), 0);

	for (auto i = 0u; i < elems.size(); i++)
		inDegree[i] = nodePred[i].size();
	return inDegree;
}

template<typename T, typename H>
template<typename FC, typename FN, typename FE>
void AdjList<T, H>::dfsUtil(NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
			 std::vector<NodeId> &p, std::vector<Timestamp> &d,
			 std::vector<Timestamp> &f,
			 FC&& onCycle, FN&& onNeighbor, FE&& atExplored)
{
	m[i] = NodeStatus::entered;
	d[i] = ++t;
	for (auto &j : nodeSucc[i]) {
		if (m[j] == NodeStatus::unseen) {
			p[j] = i;
			dfsUtil(j, t, m, p, d, f, onCycle, onNeighbor, atExplored);
		} else if (m[j] == NodeStatus::entered) {
			onCycle(i, t, m, p, d, f);
		}
		onNeighbor(i, j, t, m, p, d, f);
	}
	m[i] = NodeStatus::left;
	f[i] = ++t;
	atExplored(i, t, m, p, d, f);
	return;
}

template<typename T, typename H>
template<typename FC, typename FN, typename FE, typename FEND>
void AdjList<T, H>::dfs(FC&& onCycle, FN&& onNeighbor, FE&& atExplored, FEND&& atEnd)
{
	Timestamp t = 0;
	std::vector<NodeStatus> m(nodeSucc.size(), NodeStatus::unseen); /* Node status */
	std::vector<NodeId> p(nodeSucc.size(), -42);                    /* Node parent */
	std::vector<Timestamp> d(nodeSucc.size(), 0);                   /* First visit */
	std::vector<Timestamp> f(nodeSucc.size(), 0);                   /* Last visit */

	for (auto i = 0u; i < nodeSucc.size(); i++) {
		if (m[i] == NodeStatus::unseen)
			dfsUtil(i, t, m, p, d, f, onCycle,
				onNeighbor, atExplored);
	}
	atEnd(t, m, p, d, f);
	return;
}

template<typename T, typename H>
template<typename F>
std::vector<T> AdjList<T, H>::topoSort(F&& onSort)
{
	std::vector<T> sort;

	dfs([&](NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){ BUG(); }, /* onCycle */
	    [&](NodeId j, Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){ return; }, /* onNeighbor*/
	    [&](NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){ /* atExplored */
		    sort.push_back(elems[i]);
	    },
	    [&](Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){ return; } /* atEnd*/);
	return std::reverse(sort.begin(), sort.end());
}

template<typename T, typename H>
template<typename F>
bool AdjList<T, H>::allTopoSortUtil(std::vector<T> &current,
				 std::vector<bool> visited,
				 std::vector<int> &inDegree,
				 F&& prop, bool &found) const
{
	/* If we have already found a sorting satisfying "prop", return */
	if (found)
		return true;
	/*
	 * The boolean variable 'scheduled' indicates whether this recursive call
	 * has added (scheduled) one event (at least) to the current topological sorting.
	 * If no event was added, a full topological sort has been produced.
	 */
	auto scheduled = false;
	auto &es = getElems();

	for (auto i = 0u; i < es.size(); i++) {
		/* If ith-event can be added */
		if (inDegree[i] == 0 && !visited[i]) {
			/* Reduce in-degrees of its neighbors */
			for (auto j = 0u; j < es.size(); j++)
				if ((*this)(i, j))
					--inDegree[j];
			/* Add event in current sorting, mark as visited, and recurse */
			current.push_back(es[i]);
			visited[i] = true;

			allTopoSortUtil(current, visited, inDegree, prop, found);

			/* If the recursion yielded a sorting satisfying prop, stop */
			if (found)
				return true;

			/* Reset visited, current sorting, and inDegree */
			visited[i] = false;
			current.pop_back();
			for (auto j = 0u; j < es.size(); j++)
				if ((*this)(i, j))
					++inDegree[j];
			/* Mark that at least one event has been added to the current sorting */
			scheduled = true;
		}
	}

	/*
	 * We reach this point if no events were added in the current sorting, meaning
	 * that this is a complete sorting
	 */
	if (!scheduled) {
		if (prop(current))
			found = true;
	}
	return found;
}

template<typename T, typename H>
template<typename F>
bool AdjList<T, H>::allTopoSort(F&& prop) const
{
	std::vector<bool> visited(elems.size(), false);
	std::vector<T> current;
	auto inDegree = getInDegrees();
	auto found = false;

	return allTopoSortUtil(current, visited, inDegree, prop, found);
}

template<typename T, typename H>
template<typename F>
bool AdjList<T, H>::combineAllTopoSortUtil(unsigned int index, std::vector<std::vector<T>> &current,
					   bool &found, const std::vector<AdjList<T, H> *> &toCombine,
					   F&& prop)
{
	/* If we have found a valid combination already, return */
	if (found)
		return true;

	/* Base case: a combination of sortings has been reached */
	BUG_ON(index > toCombine.size());
	if (index == toCombine.size()) {
		if (prop(current))
			found = true;
		return found;
	}

	/* Otherwise, we have more matrices to extend */
	toCombine[index]->allTopoSort([&](std::vector<T> &sorting){
			current.push_back(sorting);
			auto res = combineAllTopoSortUtil(index + 1, current, found, toCombine, prop);
			current.pop_back();
			return res;
		});
	return found;
}

template<typename T, typename H>
template<typename F>
bool AdjList<T, H>::combineAllTopoSort(const std::vector<AdjList<T, H> *> &toCombine, F&& prop)
{
	std::vector<std::vector<T>> current; /* The current sorting for each matrix */
	bool found = false;

	return combineAllTopoSortUtil(0, current, found, toCombine, prop);
}

template<typename T, typename H>
void AdjList<T, H>::transClosure()
{
	if (calculatedTransC)
		return;

	std::vector<llvm::BitVector> indirect(elems.size(), llvm::BitVector(elems.size()));

	transC.resize(elems.size());
	for (auto i = 0u; i < elems.size(); i++) {
		transC[i].resize(elems.size());
		transC[i].reset();
	}

	dfs([&](NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){ return; }, /* onCycle */
	    [&](NodeId i, NodeId j, Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){ indirect[i] |= transC[j]; }, /* onNeighbor*/
	    [&](NodeId i, Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){
		    std::vector<NodeId> toRemove;

		    /* Also calculate the transitive reduction */
		    transC[i] = std::move(indirect[i]);
		    for (auto &j : nodeSucc[i]) {
			    if (transC[i][j])
				    toRemove.push_back(j);
			    transC[i].set(j);
		    }

		    for (auto &j : toRemove) {
			    /* Remove i from preds of transitive successors */
			    nodePred[j].erase(
				    std::remove(nodePred[j].begin(),
						nodePred[j].end(), i),
				    nodePred[j].end());

			    /* Remove transitive successors from i */
			    nodeSucc[i].erase(
				    std::remove(nodeSucc[i].begin(),
						nodeSucc[i].end(), j),
				    nodeSucc[i].end());

			    /* Also Remove edge */
			    edges.erase(std::make_pair(i, j));
		    }

	    }, /* atExplored*/
	    [&](Timestamp &t, std::vector<NodeStatus> &m,
		std::vector<NodeId> &p, std::vector<Timestamp> &d,
		std::vector<Timestamp> &f){ return; }); /* atEnd */

	calculatedTransC = true;
	return;
}

template<typename T, typename H>
llvm::raw_ostream& operator<<(llvm::raw_ostream &s, const AdjList<T, H> &l)
{
	auto &elems = l.getElems();

	s << "Elements: ";
	for (auto &e : elems)
		s << e << " ";
	s << "\n";

	for (auto i = 0u; i < elems.size(); i++) {
		s << elems[i] << " -> ";
		for (auto &j : l.nodeSucc[i])
			s << elems[j] << " ";
		s << "\n";
	}

	if (!l.calculatedTransC)
		return s;

	s << "Transitively reduced. Transitive closure:\n";
	for (auto i = 0u; i < elems.size(); i++) {
		s << elems[i] << " -> ";
		for (auto j = 0u; j < l.transC[i].size(); j++)
			if (l.transC[i][j])
				s << elems[j] << " ";
		s << "\n";
	}
	return s;
}
