#ifndef FPMAS_UNIFORM_GRAPH_BUILDER_H
#define FPMAS_UNIFORM_GRAPH_BUILDER_H

/** \file src/fpmas/graph/uniform_graph_builder.h
 * Algorithms to build random and uniform graphs.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "random_graph_builder.h"
#include "fpmas/utils/functional.h"

namespace fpmas { namespace graph {
	/**
	 * api::graph::GraphBuilder implementation that can be used to generate
	 * random uniform graphs. When links are built, nodes are selected
	 * uniformly among all the available built nodes.
	 */
	// TODO 2.0: remove or rename this, use Distributed instead
	template<typename T>
		class UniformGraphBuilder :
			public api::graph::GraphBuilder<T>,
			private RandomGraphBuilder {
				private:
					/*
					 * Function object that return a random integer between 0 and
					 * the specified argument (included).
					 */
					std::function<std::size_t(std::size_t)> index;

				public:
					/**
					 * UniformGraphBuilder constructor.
					 *
					 * @tparam Generator_t random generator type (must satisfy
					 * _UniformRandomBitGenerator_)
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 *
					 * @param generator random number generator provided to the
					 * distribution
					 * @param distribution random distribution that manages edges
					 * generation. See build()
					 */
					// TODO 2.0: Remove this constructor
					template<typename Generator_t, typename EdgeDist>
						UniformGraphBuilder(
								Generator_t& generator,
								EdgeDist& distribution) :
							RandomGraphBuilder(generator, distribution),
							// A lambda function is used to "catch" generator, without
							// requiring class level template parameters.
							index([&generator] (std::size_t max) {
									random::UniformIntDistribution<std::size_t> index(
											0, max
											);
									return index(generator);
									}) {
							}

					/**
					 * UniformGraphBuilder constructor.
					 *
					 * The RandomGraphBuilder::rd random generator is used. It
					 * can be seeded with RandomGraphBuilder::seed() or
					 * fpmas::seed().
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 *
					 * @param distribution random distribution that manages edges
					 * generation. See build()
					 */
					template<typename EdgeDist>
						UniformGraphBuilder(EdgeDist& distribution)
						: UniformGraphBuilder(RandomGraphBuilder::rd, distribution) {
						}

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
			::build(
					api::graph::NodeBuilder<T>& node_builder,
					api::graph::LayerId layer,
					api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> built_nodes;
				while(!node_builder.nodeCount() == 0)
					built_nodes.push_back(node_builder.buildNode(graph));

				std::vector<api::graph::DistributedNode<T>*> node_buffer = built_nodes;
				for(auto node : built_nodes) {
					std::size_t edge_count = std::min(
							// Random edge count
							this->num_edge(), built_nodes.size()-1
							);

					auto position = std::find(
								node_buffer.begin(), node_buffer.end(),
								node
							);
					std::swap(*position, node_buffer.back());

					for(std::size_t i = 0; i < edge_count; i++) {
						auto& target_node = node_buffer[
							this->index(node_buffer.size()-2-i) // random index
						];
						graph.link(node, target_node, layer);

						std::swap(target_node, node_buffer[node_buffer.size()-2-i]);
					}
				}
				return built_nodes;
		}

	/**
	 * Distributed version of the UniformGraphBuilder.
	 *
	 * The DistributedUniformGraphBuilder should generally be used, since it has
	 * been proved to achieve much better performances.
	 */
	template<typename T>
		class DistributedUniformGraphBuilder :
			public api::graph::DistributedGraphBuilder<T>,
			private RandomGraphBuilder {
				private:
					/*
					 * Function object that return a random integer between 0 and
					 * the specified argument (included).
					 */
					std::function<std::size_t(std::size_t)> index;
				public:
					
					/**
					 * DistributedUniformGraphBuilder constructor.
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 *
					 * @param generator random number generator provided to the
					 * distribution
					 * @param distribution random distribution that manages edges
					 * generation. See build()
					 */
					template<typename EdgeDist>
						DistributedUniformGraphBuilder(
								random::DistributedGenerator<>& generator,
								EdgeDist& distribution) :
							RandomGraphBuilder(generator, distribution),
							// A lambda function is used to "catch" generator, without
							// requiring class level template parameters.
							index([&generator] (std::size_t max) {
									random::UniformIntDistribution<std::size_t> index(
											0, max
											);
									return index(generator);
									}) {
						}

					/**
					 * DistributedUniformGraphBuilder constructor.
					 *
					 * The RandomGraphBuilder::distributed_rd random generator
					 * is used. It can be seeded with
					 * RandomGraphBuilder::seed() or fpmas::seed().
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 *
					 * @param distribution random distribution that manages edges
					 * generation. See build()
					 */
					template<typename EdgeDist>
						DistributedUniformGraphBuilder(
								EdgeDist& distribution) :
							DistributedUniformGraphBuilder(RandomGraphBuilder::distributed_rd, distribution) {
							}

					/**
					 * Same as UniformGraphBuilder::build(), but distributed and
					 * much more efficient.
					 *
					 * This process is **synchronous** and should be called from
					 * **all** processes.
					 *
					 * @param node_builder DistributedNodeBuilder instance used to generate nodes
					 * @param layer layer on which nodes will be linked
					 * @param graph graph in which nodes and edges will be inserted
					 * @return built nodes
					 */
					std::vector<api::graph::DistributedNode<T>*> build(
							api::graph::DistributedNodeBuilder<T>& node_builder,
							api::graph::LayerId layer,
							api::graph::DistributedGraph<T>& graph) override;
			};

	template<typename T>
		std::vector<api::graph::DistributedNode<T>*> DistributedUniformGraphBuilder<T>
			::build(
					api::graph::DistributedNodeBuilder<T>& node_builder,
					api::graph::LayerId layer,
					api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> built_nodes;
				while(!node_builder.localNodeCount() == 0)
					built_nodes.push_back(node_builder.buildNode(graph));

				typedef std::pair<api::graph::DistributedId, int> LocId;
				communication::TypedMpi<std::vector<LocId>>
					id_mpi(graph.getMpiCommunicator());
				std::vector<LocId> node_ids;
				for(auto node : built_nodes)
					node_ids.push_back({node->getId(), node->location()});

				node_ids = fpmas::communication::all_reduce(
						id_mpi, node_ids, fpmas::utils::Concat()
						);


				for(auto node : built_nodes) {
					std::size_t edge_count = std::min(
							this->num_edge(), node_ids.size()-1
							);

					auto position = std::find_if(
								node_ids.begin(), node_ids.end(),
								[node] (const LocId& id) {return id.first == node->getId();}
							);
					std::swap(*position, node_ids.back());

					for(std::size_t i = 0; i < edge_count; i++) {
						auto& id = node_ids[this->index(node_ids.size()-2-i)];

						fpmas::api::graph::DistributedNode<T>* target_node;
						try{
							target_node = graph.getNode(id.first);
						} catch(std::out_of_range&) {
							target_node = node_builder.buildDistantNode(id.first, id.second, graph);
						}
						graph.link(node, target_node, layer);

						std::swap(id, node_ids[node_ids.size()-2-i]);
					}
				}

				// Commits new links
				graph.synchronizationMode().getSyncLinker().synchronize();

				return built_nodes;
		}
}}
#endif
