#ifndef FPMAS_GRAPH_BUILDER_H
#define FPMAS_GRAPH_BUILDER_H

/** \file src/fpmas/graph/graph_builder.h
 * GraphBuilder implementation.
 */

#include "uniform_graph_builder.h"
#include "clustered_graph_builder.h"
#include "ring_graph_builder.h"

namespace fpmas { namespace graph {

	/**
	 * Base implementation of the DistributedNodeBuilder.
	 *
	 * This is not a final implementation: this class must be extended to
	 * implement buildNode() and buildDistantNode().
	 *
	 * @tparam T graph datatype
	 */
	template<typename T>
	class DistributedNodeBuilder : public api::graph::DistributedNodeBuilder<T> {
		protected:
			/**
			 * Current count of nodes that can be generated on the local
			 * process.
			 */
			std::size_t local_node_count;

		public:
			/**
			 * DistributedNodeBuilder constructor.
			 *
			 * The count of nodes to be built on each process
			 * (localNodeCount()) is initialized so that each process must
			 * (approximately) build the same amount of nodes.
			 *
			 * @param node_count total node count that must be generated on all
			 * processes
			 * @param comm MPI communicator
			 */
			DistributedNodeBuilder(
					std::size_t node_count,
					fpmas::api::communication::MpiCommunicator& comm) {
				local_node_count = node_count / comm.getSize();
				if(comm.getRank() == 0)
					local_node_count += node_count % comm.getSize();
			}

			/**
			 * Alias for localNodeCount()
			 */
			std::size_t nodeCount() override {
				return local_node_count;
			}

			/**
			 * \copydoc fpmas::api::graph::DistributedNodeBuilder::localNodeCount()
			 */
			std::size_t localNodeCount() override {
				return local_node_count;
			}
	};

	template<typename T>
		class DefaultDistributedNodeBuilder : public DistributedNodeBuilder<int> {
			public:
				using fpmas::graph::DistributedNodeBuilder<int>::DistributedNodeBuilder;

				fpmas::api::graph::DistributedNode<int>* buildNode(
						fpmas::api::graph::DistributedGraph<int>& graph) override {
					this->local_node_count--;
					return graph.buildNode(T());
				}

				fpmas::api::graph::DistributedNode<int>* buildDistantNode(
						fpmas::api::graph::DistributedId id,
						int location,
						fpmas::api::graph::DistributedGraph<int>& graph) override {
					auto new_node = new fpmas::graph::DistributedNode<int>(id, T());
					new_node->setLocation(location);
					return graph.insertDistant(new_node);
				}
		};


	/**
	 * api::graph::GraphBuilder implementation used to generate bipartite
	 * graphs. Built nodes are linked to nodes specified as "base nodes".
	 */
	template<typename T>
		class BipartiteGraphBuilder :
			public api::graph::GraphBuilder<T>,
			private RandomGraphBuilder {
			private:
				std::vector<api::graph::DistributedNode<T>*> base_nodes;
				/*
				 * Function object that return a random integer between 0 and
				 * the specified argument (included).
				 */
				std::function<std::size_t(std::size_t)> index;

			public:
				/**
				 * BipartiteGraphBuilder constructor.
				 *
				 * @tparam Generator_t random generator type (must satisfy
				 * _UniformRandomBitGenerator_)
				 * @tparam EdgeDist edge count distribution (must satisfy
				 * _RandomNumberDistribution_)
				 *
				 * @param generator random number generator provided to the
				 * distribution
				 * @param edge_distribution random distribution that manages edges
				 * generation. See build()
				 * @param base_nodes nodes to which built nodes will be linked
				 */
				template<typename Generator_t, typename EdgeDist>
					BipartiteGraphBuilder(
							Generator_t& generator,
							EdgeDist& edge_distribution,
							std::vector<api::graph::DistributedNode<T>*> base_nodes) :
						RandomGraphBuilder(generator, edge_distribution),
						base_nodes(base_nodes),
						index([&generator] (std::size_t max) {
								fpmas::random::UniformIntDistribution<std::size_t> index(
										0, max
										);
								return index(generator);
								}){
						}

				/**
				 * For each built node, an outgoing edge count `n` is
				 * determined by `edge_distribution`.
				 *
				 * Each node is then connected to a set of `n` nodes randomly
				 * and uniformly selected from the `base_nodes` set. If
				 * `base_nodes.size() < n`, the node is only connected to all
				 * the nodes of `base_nodes`.
				 *
				 * @param node_builder NodeBuilder instance used to generate nodes
				 * @param layer layer on which nodes will be linked
				 * @param graph graph in which nodes and edges will be inserted
				 */
				std::vector<api::graph::DistributedNode<T>*> build(
						api::graph::NodeBuilder<T>& node_builder,
						api::graph::LayerId layer,
						api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T>
		std::vector<api::graph::DistributedNode<T>*> BipartiteGraphBuilder<T>::build(
				api::graph::NodeBuilder<T>& node_builder,
				api::graph::LayerId layer,
				api::graph::DistributedGraph<T>& graph) {
			std::vector<api::graph::DistributedNode<T>*> built_nodes;

			while(node_builder.nodeCount() > 0) {
				auto* node = node_builder.buildNode(graph);
				built_nodes.push_back(node);
				std::size_t out_neighbors_count = std::min(
						base_nodes.size(),
						this->num_edge()
						);
				for(std::size_t i = 0; i < out_neighbors_count; i++) {
					auto& target_node = base_nodes[this->index(base_nodes.size()-1-i)];
					graph.link(node, target_node, layer);

					std::swap(target_node, base_nodes[base_nodes.size()-1-i]);
				}
			}
			return built_nodes;
		}
}}
#endif
