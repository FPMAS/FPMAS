#ifndef FPMAS_GRAPH_BUILDER_H
#define FPMAS_GRAPH_BUILDER_H

/** \file src/fpmas/graph/graph_builder.h
 * GraphBuilder implementation.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "fpmas/api/random/distribution.h"

namespace fpmas { namespace graph {

	/**
	 * A base class that can be used for graph builders based on a random
	 * number of outgoing edges for each node, according to the specified
	 * random distribution.
	 */
	class RandomGraphBuilder {
		protected:
			/**
			 * Random generator.
			 */
			api::random::Generator& generator;
			/**
			 * Outgoing edge count distribution.
			 */
			api::random::Distribution<std::size_t>& edge_distribution;
			/**
			 * RandomGraphBuilder constructor.
			 *
			 * @param generator random generator
			 * @param edge_distribution outgoing edge count distribution
			 */
			RandomGraphBuilder(
					api::random::Generator& generator,
					api::random::Distribution<std::size_t>& edge_distribution)
				: generator(generator), edge_distribution(edge_distribution) {}
	};


	/**
	 * api::graph::GraphBuilder implementation that can be used to generate
	 * random uniform graphs. When links are built, nodes are selected
	 * uniformly among all the available built nodes.
	 */
	template<typename T>
		class UniformGraphBuilder : public api::graph::GraphBuilder<T>, private RandomGraphBuilder {
			public:
				/**
				 * UniformGraphBuilder constructor.
				 *
				 * @param generator random number generator provided to the
				 * distribution
				 * @param distribution random distribution that manages edges
				 * generation. See build()
				 */
				UniformGraphBuilder(
						api::random::Generator& generator,
						api::random::Distribution<std::size_t>& distribution)
					: RandomGraphBuilder(generator, distribution) {}

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
				 * @return built nodes
				 */
				std::vector<api::graph::DistributedNode<T>*> build(
						api::graph::NodeBuilder<T>& node_builder,
						api::graph::LayerId layer,
						api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T>
		std::vector<api::graph::DistributedNode<T>*> UniformGraphBuilder<T>
			::build(api::graph::NodeBuilder<T>& node_builder, api::graph::LayerId layer, api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> built_nodes;
				while(!node_builder.nodeCount() == 0)
					built_nodes.push_back(node_builder.buildNode(graph));

				for(auto node : built_nodes) {
					std::size_t edge_count = std::min(edge_distribution(generator), built_nodes.size()-1);

					std::vector<api::graph::DistributedNode<T>*> available_nodes = built_nodes;
					available_nodes.erase(std::remove(available_nodes.begin(), available_nodes.end(), node));

					std::shuffle(available_nodes.begin(), available_nodes.end(), generator);
					for(std::size_t i = 0; i < edge_count; i++) {
						graph.link(node, available_nodes.at(i), layer);
					}
				}
				return built_nodes;
		}

	/**
	 * api::graph::GraphBuilder implementation that can be used to generate
	 * random clustered graphs. A random 2D coordinate is implicitly associated
	 * to each node : each node is then connected to its nearest neighbors.
	 */
	template<typename T>
		class ClusteredGraphBuilder : public api::graph::GraphBuilder<T>, private RandomGraphBuilder {
			private:
				api::random::Distribution<double>& x_distribution;
				api::random::Distribution<double>& y_distribution;

				struct LocalizedNode {
					double x;
					double y;
					api::graph::DistributedNode<T>* node;

					LocalizedNode(double x, double y, api::graph::DistributedNode<T>* node)
						: x(x), y(y), node(node) {}

					bool operator==(const LocalizedNode& node) const {
						return node.node == this->node;
					}
					static double distance(const LocalizedNode& n1, const LocalizedNode& n2) {
						return std::pow(n2.y - n1.y, 2) + std::pow(n2.x - n1.x, 2);
					}
				};

				class Comparator {
					LocalizedNode& node;
					public:
					Comparator(LocalizedNode& node)
						: node(node) {}

					bool operator()(const LocalizedNode& n1, const LocalizedNode& n2) {
						return LocalizedNode::distance(n1, node) < LocalizedNode::distance(n2, node);
					}
				};

			public:
				/**
				 * ClusteredGraphBuilder constructor.
				 *
				 * @param generator random number generator provided to the
				 * distribution
				 * @param edge_distribution random distribution that manages edges
				 * generation. See build()
				 * @param x_distribution random distribution used to assign an
				 * x coordinate to each node
				 * @param y_distribution random distribution used to assign an
				 * y coordinate to each node
				 */
				ClusteredGraphBuilder(
						api::random::Generator& generator,
						api::random::Distribution<std::size_t>& edge_distribution,
						api::random::Distribution<double>& x_distribution,
						api::random::Distribution<double>& y_distribution
						) :
					RandomGraphBuilder(generator, edge_distribution),
					x_distribution(x_distribution),
					y_distribution(y_distribution)
			{}

				/**
				 * A 2D coordinate is first assigned to each node provided by
				 * the node_builder, according to the specified
				 * `x_distribution` and `y_distribution`.
				 *
				 * Then, for each node, an outgoing edge count `n` is determined by
				 * `edge_distribution`. The node is then connected to its `n`
				 * nearest neighbors according to the previously computed
				 * coordinates.
				 *
				 * Notice that those coordinates are completely implicit and
				 * independent from the kind of nodes actually built by the
				 * algorithm.
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
		std::vector<api::graph::DistributedNode<T>*> ClusteredGraphBuilder<T>
			::build(api::graph::NodeBuilder<T>& node_builder, api::graph::LayerId layer, api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> raw_built_nodes;
				std::vector<LocalizedNode> built_nodes;
				while(!node_builder.nodeCount() == 0) {
					auto* node = node_builder.buildNode(graph);
					built_nodes.push_back({
							x_distribution(generator), y_distribution(generator),
							node});
					raw_built_nodes.push_back(node);
				}

				for(auto& node : built_nodes) {
					std::size_t edge_count = std::min(edge_distribution(generator), built_nodes.size()-1);

					std::vector<LocalizedNode> available_nodes = built_nodes;
					available_nodes.erase(std::remove(available_nodes.begin(), available_nodes.end(), node));

					Comparator comp(node);
					std::sort(available_nodes.begin(), available_nodes.end(), comp);
					for(std::size_t i = 0; i < std::min(edge_count, available_nodes.size()); i++) {
						graph.link(node.node, available_nodes[i].node, layer);
					}
				}
				return raw_built_nodes;
			}

	/**
	 * api::graph::GraphBuilder implementation used to generate bipartite
	 * graphs. Built nodes are linked to nodes specified as "base nodes".
	 */
	template<typename T>
		class BipartiteGraphBuilder : public api::graph::GraphBuilder<T>, private RandomGraphBuilder {
			private:
				std::vector<api::graph::DistributedNode<T>*> base_nodes;
			public:
				/**
				 * BipartiteGraphBuilder constructor.
				 *
				 * @param generator random number generator provided to the
				 * distribution
				 * @param edge_distribution random distribution that manages edges
				 * generation. See build()
				 * @param base_nodes nodes to which built nodes will be linked
				 */
				BipartiteGraphBuilder(
						api::random::Generator& generator,
						api::random::Distribution<std::size_t>& edge_distribution,
						std::vector<api::graph::DistributedNode<T>*> base_nodes)
					: RandomGraphBuilder(generator, edge_distribution), base_nodes(base_nodes) {}

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
						edge_distribution(generator));
				std::vector<api::graph::DistributedNode<T>*> shuffled_base_nodes = base_nodes;
				std::shuffle(shuffled_base_nodes.begin(), shuffled_base_nodes.end(), generator);

				for(std::size_t i = 0; i < out_neighbors_count; i++) {
					graph.link(node, base_nodes[i], layer);
				}
			}
			return built_nodes;
		}
}}
#endif
