#ifndef FPMAS_GRAPH_BUILDER_H
#define FPMAS_GRAPH_BUILDER_H

/** \file src/fpmas/graph/graph_builder.h
 * GraphBuilder implementation.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "fpmas/api/random/distribution.h"

namespace fpmas { namespace graph {

	/**
	 * api::graph::GraphBuilder implementation that can be used to generate
	 * random graphs.
	 */
	template<typename T>
		class RandomDegreeGraphBuilder : public api::graph::GraphBuilder<T> {
			private:
				api::random::Generator& generator;
				api::random::Distribution<std::size_t>& distribution;

			public:
				/**
				 * RandomDegreeGraphBuilder constructor.
				 *
				 * @param generator random number generator provided to the
				 * distribution
				 * @param distribution random distribution that manages edges generation.
				 * See build()
				 */
				RandomDegreeGraphBuilder(
						api::random::Generator& generator,
						api::random::Distribution<std::size_t>& distribution)
					: generator(generator), distribution(distribution) {}

				/**
				 * Builds a graph according to the following algorithm.
				 *
				 * 1. api::graph::NodeBuilder::nodeCount() are built in the
				 * graph.
				 * 2. The number of outgoing edge of each node is determined by
				 * the value returned by `distribution(generator)`.
				 * 3. For each node, a subset of nodes among the nodes built by
				 * the algorithm is selected uniformly, of size equal to the
				 * previously computed output degree of the node. The node is
				 * then linked to each node of the subset, on the specified
				 * `layer`.
				 *
				 * Any random distribution can be used to build such a graph.
				 * The mean output degree of the graph, K, will by definition
				 * be equal to the mean of the specified random distribution.
				 *
				 * @param node_builder NodeBuilder instance used to generate nodes
				 * @param layer layer on which nodes will be linked
				 * @param graph graph in which nodes and edges will be inserted
				 */
				void build(
						api::graph::NodeBuilder<T>& node_builder,
						api::graph::LayerId layer,
						api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T>
		void RandomDegreeGraphBuilder<T>
			::build(api::graph::NodeBuilder<T>& node_builder, api::graph::LayerId layer, api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> built_nodes;
				while(!node_builder.nodeCount() == 0)
					built_nodes.push_back(node_builder.buildNode(graph));

				for(auto node : built_nodes) {
					std::size_t edge_count = std::min(distribution(generator), built_nodes.size()-1);

					std::vector<api::graph::DistributedNode<T>*> available_nodes = built_nodes;
					available_nodes.erase(std::remove(available_nodes.begin(), available_nodes.end(), node));

					std::shuffle(available_nodes.begin(), available_nodes.end(), generator);
					for(std::size_t i = 0; i < edge_count; i++) {
						graph.link(node, available_nodes.at(i), layer);
					}
				}
		}
}}
#endif
