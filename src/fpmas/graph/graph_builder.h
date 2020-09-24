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
		class UniformGraphBuilder : public api::graph::GraphBuilder<T> {
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
				UniformGraphBuilder(
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
		void UniformGraphBuilder<T>
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

	template<typename T>
		class ClusteredGraphBuilder : public api::graph::GraphBuilder<T> {
			private:
				api::random::Generator& generator;
				api::random::Distribution<std::size_t>& edge_distribution;
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
				ClusteredGraphBuilder(
						api::random::Generator& generator,
						api::random::Distribution<std::size_t>& edge_distribution,
						api::random::Distribution<double>& x_distribution,
						api::random::Distribution<double>& y_distribution
						) :
					generator(generator),
					edge_distribution(edge_distribution),
					x_distribution(x_distribution),
					y_distribution(y_distribution)
			{}

			void build(
					api::graph::NodeBuilder<T>& node_builder,
					api::graph::LayerId layer,
					api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T>
		void ClusteredGraphBuilder<T>
			::build(api::graph::NodeBuilder<T>& node_builder, api::graph::LayerId layer, api::graph::DistributedGraph<T>& graph) {
				std::vector<LocalizedNode> built_nodes;
				while(!node_builder.nodeCount() == 0) {
					built_nodes.push_back({
							x_distribution(generator), y_distribution(generator),
							node_builder.buildNode(graph)});
				}

				for(auto& node : built_nodes) {
					std::size_t edge_count = std::min(edge_distribution(generator), built_nodes.size()-1);

					std::vector<LocalizedNode> available_nodes = built_nodes;
					available_nodes.erase(std::remove(available_nodes.begin(), available_nodes.end(), node));

					Comparator comp(node);
					std::vector<LocalizedNode> closest_neighbors;
					for(std::size_t i = 0; i < std::min(edge_count, available_nodes.size()); i++) {
						closest_neighbors.push_back(available_nodes.back());
						available_nodes.pop_back();
					}
					std::sort(closest_neighbors.begin(), closest_neighbors.end(), comp);

					for(auto& available_node : available_nodes) {
						closest_neighbors.push_back(available_node);
						std::sort(closest_neighbors.begin(), closest_neighbors.end(), comp);

						closest_neighbors.pop_back();
					}

					for(auto& closest_neighbor : closest_neighbors) {
						graph.link(node.node, closest_neighbor.node, layer);
					}
				}
			}

}}
#endif
