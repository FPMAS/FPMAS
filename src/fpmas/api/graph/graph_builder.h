#ifndef FPMAS_GRAPH_BUILDER_API_H
#define FPMAS_GRAPH_BUILDER_API_H

/** \file src/fpmas/api/graph/graph_builder.h
 * GraphBuilder API.
 */

#include "distributed_graph.h"

namespace fpmas { namespace api { namespace graph {

	/**
	 * An utility class used to automatically generate some nodes, that can be
	 * provided to a GraphBuilder.
	 */
	template<typename T>
		class NodeBuilder {
			public:
				/**
				 * Returns the count of node that the NodeBuilder can generate.
				 *
				 * The value returned by this function is expected to be
				 * updated while buildNode() calls are performed.
				 *
				 * @return count of nodes that can be built
				 */
				virtual std::size_t nodeCount() = 0;

				/**
				 * Builds a node in the specified graph.
				 *
				 * @param graph graph in which the node must be built
				 * @return pointer to built node
				 */
				virtual DistributedNode<T>* buildNode(DistributedGraph<T>& graph) = 0;
		};

	/**
	 * An utility interface used to automatically generate some graphs.
	 */
	template<typename T>
	class GraphBuilder {
		public:
			/**
			 * Automatically builds the specified graph according to the
			 * current implementation.
			 *
			 * Nodes are generated using the specified NodeBuilder. More
			 * precisely, NodeBuilder::nodeCount() nodes will be inserted in
			 * the graph. Generated nodes can then be linked on the specified
			 * layer, according to rules defined by the implemented algorithm.
			 *
			 * Notice that the specified graph is not required to be empty.
			 *
			 * @param node_builder NodeBuilder instance used to generate nodes
			 * @param layer layer on which nodes will be linked
			 * @param graph graph in which nodes and edges will be inserted
			 * @return built nodes
			 */
			virtual std::vector<api::graph::DistributedNode<T>*> build(
					NodeBuilder<T>& node_builder,
					LayerId layer,
					DistributedGraph<T>& graph) = 0;
	};
}}}
#endif
