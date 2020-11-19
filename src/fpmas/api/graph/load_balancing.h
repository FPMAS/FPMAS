#ifndef FPMAS_LOAD_BALANCING_API_H
#define FPMAS_LOAD_BALANCING_API_H

/** \file src/fpmas/api/graph/load_balancing.h
 * LoadBalancing API
 */

#include "fpmas/api/graph/graph.h"
#include "fpmas/api/graph/distributed_node.h"

namespace fpmas { namespace api { namespace graph {
	/**
	 * Type used to describe a DistributedNode partition.
	 *
	 * The PartitionMap associates node IDs to the rank of the process to which
	 * they should be assigned.
	 */
	typedef std::unordered_map<DistributedId, int, api::graph::IdHash<DistributedId>> PartitionMap;

	/**
	 * Type used to represent nodes to distribute.
	 */
	template<typename T>
	using NodeMap = typename graph::Graph<graph::DistributedNode<T>, graph::DistributedEdge<T>>::NodeMap;

	/**
	 * Load balancing API with fixed vertices handling.
	 */
	template<typename T>
		class FixedVerticesLoadBalancing {
			public:
				/**
				 * Computes a node partition from the input nodes, assigning a
				 * fixed rank to nodes specified in the fixed vertices map.
				 *
				 * This functions is synchronous and blocks until all processes
				 * call it.
				 *
				 * Each process calls the functions with its own node map, so
				 * that the global set of nodes to balance correspond to the
				 * union of all the local node maps specified as arguments.
				 *
				 * Fixed vertices are assumed to be consistent across
				 * processes, behavior is undefined otherwise.
				 *
				 * @param nodes local nodes to balance
				 * @param fixed_vertices fixed vertices map
				 * @return balanced partition map
				 */
				virtual PartitionMap balance(
						NodeMap<T> nodes,
						PartitionMap fixed_vertices
						) = 0;

				virtual ~FixedVerticesLoadBalancing() {}
		};

	/**
	 * Load balancing API.
	 */
	template<typename T>
		class LoadBalancing {
			public:
				/**
				 * Computes a node partition from the input nodes.
				 *
				 * This functions is synchronous and blocks until all processes
				 * call it.
				 *
				 * Each process calls the functions with its own node map, so
				 * that the global set of nodes to balance correspond to the
				 * union of all the local node maps specified as arguments.
				 *
				 * @param nodes local nodes to balance
				 * @return balanced partition map
				 */
				virtual PartitionMap balance(NodeMap<T> nodes) = 0;

				virtual ~LoadBalancing() {}
		};

}}}
#endif
