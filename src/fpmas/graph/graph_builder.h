#ifndef FPMAS_GRAPH_BUILDER_H
#define FPMAS_GRAPH_BUILDER_H

/** \file src/fpmas/graph/graph_builder.h
 * GraphBuilder implementation.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "fpmas/api/random/distribution.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/distributed_node.h"
#include "fpmas/random/random.h"
#include "fpmas/utils/functional.h"

#include <list>

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
							edge_distribution(generator), built_nodes.size()-1
							);

					auto position = std::find(
								node_buffer.begin(), node_buffer.end(),
								node
							);
					std::swap(*position, node_buffer.back());

					for(std::size_t i = 0; i < edge_count; i++) {
						fpmas::random::UniformIntDistribution<std::size_t> index(
								0, node_buffer.size()-2-i
								);
						auto& target_node = node_buffer[index(generator)];
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
			public api::graph::DistributedGraphBuilder<T>, private RandomGraphBuilder {

			public:
				/**
				 * \copydoc UniformGraphBuilder::UniformGraphBuilder()
				 */
				DistributedUniformGraphBuilder(
						random::DistributedGenerator<>& generator,
						api::random::Distribution<std::size_t>& distribution)
					: RandomGraphBuilder(generator, distribution) {}

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
							edge_distribution(generator), node_ids.size()-1
							);

					auto position = std::find_if(
								node_ids.begin(), node_ids.end(),
								[node] (const LocId& id) {return id.first == node->getId();}
							);
					std::swap(*position, node_ids.back());

					for(std::size_t i = 0; i < edge_count; i++) {
						fpmas::random::UniformIntDistribution<std::size_t> index(
								0, node_ids.size()-2-i
								);
						auto& id = node_ids[index(generator)];

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

	namespace detail {
		/**
		 * Represents 2D coordinates.
		 */
		struct Point {
			/**
			 * x coordinate
			 */
			double x;
			/**
			 * y coordinate
			 */
			double y;

			/**
			 * Point constructor.
			 *
			 * @param x x coordinate
			 * @param y y coordinate
			 */
			Point(double x, double y) : x(x), y(y) {}

			/**
			 * Computes the Euclidian distance between two Points.
			 *
			 * @param p1 first point
			 * @param p2 second point
			 * @return euclidian distance between `p1` and `p2`
			 */
			static double distance(const Point& p1, const Point& p2) {
				return std::pow(p2.y - p1.y, 2) + std::pow(p2.x - p1.x, 2);
			}
		};

		/**
		 * A structure used to attach a 2D coordinate to a regular node.
		 */
		template<typename T>
			struct LocalizedNode {
				/**
				 * Node coordinates
				 */
				Point p;
				/**
				 * Pointer to existing node.
				 */
				api::graph::DistributedNode<T>* node;

				/**
				 * LocalizedNode constructor.
				 *
				 * @param p node coordinates
				 * @param node pointer to existing node
				 */
				LocalizedNode(Point p, api::graph::DistributedNode<T>* node)
					: p(p), node(node) {}
			};

		/**
		 * A structure used to represent a LocalizedNode, representing the node
		 * only by its id.
		 *
		 * This allows to represent a LocalizedNode for nodes that not located
		 * on the current process.
		 */
		template<typename T>
			struct LocalizedNodeView {
				/**
				 * Node coordinates.
				 */
				Point p;
				/**
				 * Id of the represented node.
				 */
				fpmas::api::graph::DistributedId node_id;
				/**
				 * Rank of the process that currently own the represented node.
				 */
				int location;

				/**
				 * LocalizedNodeView constructor.
				 *
				 * @param p node coordinates
				 * @param node_id id of the represented node
				 * @param location rank of the process that currently own the
				 * represented node
				 */
				LocalizedNodeView(
						Point p, 
						fpmas::api::graph::DistributedId node_id,
						int location)
					: p(p), node_id(node_id), location(location) {
					}
			};


		/**
		 * A comparator that can be used to compare distances from a reference
		 * Point.
		 */
		class DistanceComparator {
			private:
				Point& p;
			public:
				/**
				 * DistanceComparator constructor.
				 *
				 * @param p reference point
				 */
				DistanceComparator(Point& p)
					: p(p) {}

				/**
				 * Returns true if and only if the distance from `p1` to the
				 * reference point is lower than the distance from `p2` to the
				 * reference point.
				 *
				 * @return true iff `p1` is closer to the reference point than
				 * `p2
				 */
				bool operator()(const Point& p1, const Point& p2) {
					return Point::distance(p1, p) < Point::distance(p2, p);
				}
		};
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

				static random::UniformRealDistribution<double> default_xy_dist;
	
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
					y_distribution(y_distribution) {
					}

				/**
				 * ClusteredGraphBuilder constructor.
				 *
				 * Built nodes are implicitly and uniformly distributed in a 2D
				 * space.
				 *
				 * @param generator random number generator provided to the
				 * distribution
				 * @param edge_distribution random distribution that manages edges
				 * generation. See build()
				 */
				ClusteredGraphBuilder(
						api::random::Generator& generator,
						api::random::Distribution<std::size_t>& edge_distribution
						) :
					ClusteredGraphBuilder(
							generator, edge_distribution,
							default_xy_dist, default_xy_dist) {
					}

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
		random::UniformRealDistribution<double>
		ClusteredGraphBuilder<T>::default_xy_dist {0, 1000};

	template<typename T>
		std::vector<api::graph::DistributedNode<T>*> ClusteredGraphBuilder<T>
			::build(api::graph::NodeBuilder<T>& node_builder, api::graph::LayerId layer, api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> raw_built_nodes;
				std::vector<detail::LocalizedNode<T>> built_nodes;
				while(!node_builder.nodeCount() == 0) {
					auto* node = node_builder.buildNode(graph);
					built_nodes.push_back({
							{x_distribution(generator), y_distribution(generator)},
							node
							});
					raw_built_nodes.push_back(node);
				}

				for(auto& node : built_nodes) {
					std::size_t edge_count = std::min(
							edge_distribution(generator), built_nodes.size()-1
							);

					detail::DistanceComparator comp(node.p);
					std::list<detail::LocalizedNode<T>> nearest_nodes;
					std::size_t i = 0;
					while(nearest_nodes.size() < edge_count) {
						auto local_node = built_nodes[i];
						if(local_node.node != node.node) {
							auto it = nearest_nodes.begin();
							while(it != nearest_nodes.end() && comp((*it).p, local_node.p))
								it++;
							nearest_nodes.insert(it, local_node);
						}
						i++;
					}

					for(std::size_t j = i; j < built_nodes.size(); j++) {
						auto local_node = built_nodes[j];
						if(local_node.node != node.node) {
							auto it = nearest_nodes.begin();
							while(it != nearest_nodes.end() && comp((*it).p, local_node.p))
								it++;
							if(it != nearest_nodes.end()) {
								nearest_nodes.insert(it, local_node);
								nearest_nodes.pop_back();
							}
						}
					}

					for(auto nearest_node : nearest_nodes)
						graph.link(node.node, nearest_node.node, layer);
				}
				return raw_built_nodes;
			}

	/**
	 * Distributed version of the ClusteredGraphBuilder.
	 *
	 * The DistributedClusteredGraphBuilder should generally be used, since it has
	 * been proved to achieve much better performances.
	 */
	template<typename T>
		class DistributedClusteredGraphBuilder :
			public api::graph::DistributedGraphBuilder<T>,
			private RandomGraphBuilder {
			private:
				api::random::Distribution<double>& x_distribution;
				api::random::Distribution<double>& y_distribution;

				static random::UniformRealDistribution<double> default_xy_dist;
	
			public:
				/**
				 * \copydoc ClusteredGraphBuilder::ClusteredGraphBuilder()
				 */
				DistributedClusteredGraphBuilder(
						random::DistributedGenerator<>& generator,
						api::random::Distribution<std::size_t>& edge_distribution,
						api::random::Distribution<double>& x_distribution,
						api::random::Distribution<double>& y_distribution
						) :
					RandomGraphBuilder(generator, edge_distribution),
					x_distribution(x_distribution),
					y_distribution(y_distribution){
					}

				/**
				 * \copydoc ClusteredGraphBuilder::ClusteredGraphBuilder(api::random::Generator&, api::random::Distribution<std::size_t>&)
				 */
				DistributedClusteredGraphBuilder(
						random::DistributedGenerator<>& generator,
						api::random::Distribution<std::size_t>& edge_distribution
						) :
					DistributedClusteredGraphBuilder(
							generator, edge_distribution,
							default_xy_dist, default_xy_dist){
					}

				/**
				 * Same as ClusteredGraphBuilder::build(), but distributed and
				 * much more efficient.
				 *
				 * This process is **synchronous** and should be called from
				 * **all** processes.
				 *
				 * @param node_builder DistributedNodeBuilder instance used to generate nodes
				 * @param layer layer on which nodes will be linked
				 * @param graph graph in which nodes and edges will be inserted
				 */
				std::vector<api::graph::DistributedNode<T>*> build(
						api::graph::DistributedNodeBuilder<T>& node_builder,
						api::graph::LayerId layer,
						api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T>
		random::UniformRealDistribution<double>
		DistributedClusteredGraphBuilder<T>::default_xy_dist {0, 1000};

	template<typename T>
		std::vector<api::graph::DistributedNode<T>*> DistributedClusteredGraphBuilder<T>
			::build(
					api::graph::DistributedNodeBuilder<T>& node_builder,
					api::graph::LayerId layer,
					api::graph::DistributedGraph<T>& graph) {
				std::vector<api::graph::DistributedNode<T>*> raw_built_nodes;
				std::vector<detail::LocalizedNode<T>> built_nodes;
				while(!node_builder.localNodeCount() == 0) {
					auto* node = node_builder.buildNode(graph);
					built_nodes.push_back({
							{x_distribution(generator), y_distribution(generator)},
							node
							});
					raw_built_nodes.push_back(node);
				}
				std::vector<detail::LocalizedNodeView<T>> built_nodes_buffer;
				for(auto node : built_nodes)
					built_nodes_buffer.push_back({
							node.p, node.node->getId(), node.node->location()
							});

				fpmas::communication::TypedMpi<std::vector<detail::LocalizedNodeView<T>>> mpi(
						graph.getMpiCommunicator()
						);

				built_nodes_buffer = fpmas::communication::all_reduce(
						mpi, built_nodes_buffer, fpmas::utils::Concat()
						);

				for(auto& node : built_nodes) {
					std::size_t edge_count = std::min(
							edge_distribution(generator), built_nodes_buffer.size()-1
							);
					detail::DistanceComparator comp(node.p);
					std::list<detail::LocalizedNodeView<T>> nearest_nodes;
					std::size_t i = 0;
					while(nearest_nodes.size() < edge_count) {
						auto local_node = built_nodes_buffer[i];
						if(local_node.node_id != node.node->getId()) {
							auto it = nearest_nodes.begin();
							while(it != nearest_nodes.end() && comp((*it).p, local_node.p))
								it++;
							nearest_nodes.insert(it, local_node);
						}
						i++;
					}

					for(std::size_t j = i; j < built_nodes_buffer.size(); j++) {
						auto local_node = built_nodes_buffer[j];
						if(local_node.node_id != node.node->getId()) {
							auto it = nearest_nodes.begin();
							while(it != nearest_nodes.end() && comp((*it).p, local_node.p))
								it++;
							if(it != nearest_nodes.end()) {
								nearest_nodes.insert(it, local_node);
								nearest_nodes.pop_back();
							}
						}
					}

					for(auto nearest_node : nearest_nodes) {
						api::graph::DistributedNode<T>* target_node;
						try {
							target_node = graph.getNode(nearest_node.node_id);
						} catch(const std::out_of_range&) {
							target_node = node_builder.buildDistantNode(
									nearest_node.node_id, nearest_node.location, graph
									);
						}
						graph.link(node.node, target_node, layer);
					}
				}

				// Commits new links
				graph.synchronizationMode().getSyncLinker().synchronize();

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

namespace nlohmann {
	/**
	 * fpmas::graph::detail::LocalizedNodeView json serializer
	 */
	template<typename T>
		struct adl_serializer<fpmas::graph::detail::LocalizedNodeView<T>> {

			/**
			 * Serializes the provided LocalizedNodeView instance into the
			 * json `j`.
			 *
			 * @param j json output
			 * @param node LocalizedNodeView to serialize
			 */
			template<typename JsonType>
				static void to_json(
						JsonType& j,
						const fpmas::graph::detail::LocalizedNodeView<T>& node
						) {
					j = {{node.p.x, node.p.y}, node.node_id, node.location};
				}

			/**
			 * Unserializes a LocalizedNodeView instance from the json `j`.
			 *
			 * @param j json input
			 * @return unserialized LocalizedNodeView
			 */
			template<typename JsonType>
				static fpmas::graph::detail::LocalizedNodeView<T> from_json(const JsonType& j) {
					return {
						{
							j.at(0).at(0).template get<double>(),
							j.at(0).at(1).template get<double>()
						},
						j[1].template get<fpmas::api::graph::DistributedId>(),
						j.at(2).template get<int>()
					};
				}
		};
}
#endif
