#ifndef FPMAS_GRAPH_BUILDER_API_H
#define FPMAS_GRAPH_BUILDER_API_H

/** \file src/fpmas/api/graph/graph_builder.h
 * GraphBuilder API.
 */

#include "distributed_graph.h"

namespace fpmas { namespace api { namespace graph {

	// TODO 2.0: refactor all of this... GraphBuilder and
	// DistributedGraphBuilder does not need to be distinct.
	// GraphBuilder is obsolete anyway.

	/**
	 * An utility class used to automatically generate some nodes, that can be
	 * provided to a GraphBuilder.
	 *
	 * @tparam T graph datatype
	 */
	template<typename T>
		class NodeBuilder {
			public:
				/**
				 * Returns the count of nodes that the NodeBuilder can generate.
				 *
				 * The value returned by this function is expected to be
				 * updated while buildNode() calls are performed.
				 *
				 * @return count of nodes that can be built
				 */
				// TODO 2.0: refactor this
				virtual std::size_t nodeCount() = 0;

				/**
				 * Builds a node in the specified graph.
				 *
				 * @param graph graph in which the node must be built
				 * @return pointer to built node
				 */
				virtual DistributedNode<T>* buildNode(DistributedGraph<T>& graph) = 0;

				virtual ~NodeBuilder() {}
		};

	/**
	 * An utility interface used to automatically generate some graphs on a
	 * single process.
	 *
	 * @tparam T graph datatype
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

				virtual ~GraphBuilder() {}
		};

	/**
	 * Distributed extension of the NodeBuilder.
	 *
	 * The purpose of the DistributedNodeBuilder is similar to the NodeBuilder,
	 * but adds distribution specific features.
	 *
	 * @tparam T graph datatype
	 */
	template<typename T>
		class DistributedNodeBuilder : public NodeBuilder<T> {
			public:
				/**
				 * Returns the count of nodes that the DistributedNodeBuilder
				 * can generate **on the current process**.
				 *
				 * The value returned by this function is expected to be
				 * updated while buildNode() calls are performed.
				 *
				 * @return count of nodes that can be built on the local
				 * process
				 */
				virtual std::size_t localNodeCount() = 0;

				/**
				 * Builds a **distant** node in the specified `graph`, with the
				 * provided `id`. The node is assumed to be currently owned by
				 * the process with rank `location`.
				 *
				 * Such a node can be considered as a "temporary" node, that
				 * can be used by the underlying algorithm to build edges with
				 * nodes that are currently not located on this process.
				 *
				 * Implementations are likely to behave as follow:
				 * 1. Dynamically allocates a default DistributedNode
				 * 2. Initiates its location using
				 * DistributedNode::setLocation()
				 * 3. Inserts it into the graph using
				 * DistributedGraph::insertDistant()
				 *
				 * Notice that the count of nodes built using this method is
				 * not limited and is not correlated to localNodeCount() or
				 * nodeCount(), contrary to the buildNode() method.
				 *
				 * @param id id of the distant node
				 * @param location current node location
				 * @param graph graph in which the distant node will be
				 * inserted
				 */
				virtual DistributedNode<T>* buildDistantNode(
						DistributedId id, int location, DistributedGraph<T>& graph) = 0;

				virtual ~DistributedNodeBuilder() {}
		};

	/**
	 * Distributed version of the GraphBuilder.
	 *
	 * Contrary to the local version, the DistributedGraphBuilder is assumed to
	 * build a graph using a distributed algorithm, to improve performance and
	 * scalability.
	 *
	 * @tparam T graph datatype
	 */
	template<typename T>
		class DistributedGraphBuilder {
			public:
				/**
				 * Automatically builds the specified graph according to the
				 * current implementation.
				 *
				 * Nodes are generated using the specified
				 * DistributedNodeBuilder. More precisely,
				 * DistributedNodeBuilder::localNodeCount() nodes will be
				 * inserted in the graph. Generated nodes can then be linked on
				 * the specified layer, according to rules defined by the
				 * implemented algorithm.
				 *
				 * Notice that the specified graph is not required to be empty.
				 *
				 * Contrary to the GraphBuilder, this process is
				 * **synchronous** and should be called from **all** processes.
				 *
				 * @param node_builder DistributedNodeBuilder instance used to
				 * generate nodes
				 * @param layer layer on which nodes will be linked
				 * @param graph graph in which nodes and edges will be inserted
				 * @return built nodes
				 */
				virtual std::vector<api::graph::DistributedNode<T>*> build(
						DistributedNodeBuilder<T>& node_builder,
						LayerId layer,
						DistributedGraph<T>& graph) = 0;

				virtual ~DistributedGraphBuilder() {}
		};
}}}
#endif
