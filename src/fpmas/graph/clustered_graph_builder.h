#ifndef FPMAS_CLUSTERED_GRAPH_BUILDER_H
#define FPMAS_CLUSTERED_GRAPH_BUILDER_H

/** \file src/fpmas/graph/clustered_graph_builder.h
 * Algorithms to build clustered graphs.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "random_graph_builder.h"
#include "fpmas/utils/functional.h"

#include <list>

namespace fpmas { namespace graph {
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
		class ClusteredGraphBuilder :
			public api::graph::GraphBuilder<T>,
			private RandomGraphBuilder {
				private:
					/*
					 * Function object that returns a random x coordinate.
					 */
					std::function<double()> x;
					/*
					 * Function object that returns a random y coordinate.
					 */
					std::function<double()> y;

					static random::UniformRealDistribution<double> default_xy_dist;

				public:
					/**
					 * ClusteredGraphBuilder constructor.
					 *
					 * @tparam Generator_t random generator type (must satisfy
					 * _UniformRandomBitGenerator_)
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 * @tparam X_Dist x coordinates distribution (must satisfy
					 * RandomNumberDistribution)
					 * @tparam Y_Dist y coordinates distribution (must satisfy
					 * RandomNumberDistribution)
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
					template<typename Generator_t, typename EdgeDist, typename X_Dist, typename Y_Dist>
						ClusteredGraphBuilder(
								Generator_t& generator,
								EdgeDist& edge_distribution,
								X_Dist& x_distribution,
								Y_Dist& y_distribution
								) :
							RandomGraphBuilder(generator, edge_distribution),
							// Generator, x_distribution and y_distribution are
							// catched in a lambda to avoid class template
							// parameters usage
							x([&generator, &x_distribution] () {
									return x_distribution(generator);
									}),
							y([&generator, &y_distribution] () {
									return y_distribution(generator);
									}) {
							}

					/**
					 * ClusteredGraphBuilder constructor.
					 *
					 * The RandomGraphBuilder::rd random generator is used. It can
					 * be seeded with RandomGraphBuilder::seed() or fpmas::seed().
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 * @tparam X_Dist x coordinates distribution (must satisfy
					 * RandomNumberDistribution)
					 * @tparam Y_Dist y coordinates distribution (must satisfy
					 * RandomNumberDistribution)
					 *
					 * @param edge_distribution random distribution that manages edges
					 * generation. See build()
					 * @param x_distribution random distribution used to assign an
					 * x coordinate to each node
					 * @param y_distribution random distribution used to assign an
					 * y coordinate to each node
					 */
					template<typename EdgeDist, typename X_Dist, typename Y_Dist>
						ClusteredGraphBuilder(
								EdgeDist& edge_distribution,
								X_Dist& x_distribution,
								Y_Dist& y_distribution
								) : ClusteredGraphBuilder(
									RandomGraphBuilder::rd, edge_distribution,
									x_distribution, y_distribution) {
								}

					/**
					 * ClusteredGraphBuilder constructor.
					 *
					 * Built nodes are implicitly and uniformly distributed in a 2D
					 * space.
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
					 */
					template<typename Generator_t, typename Distribution_t>
						ClusteredGraphBuilder(
								Generator_t& generator,
								Distribution_t& edge_distribution
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
			while(node_builder.nodeCount() != 0) {
				auto* node = node_builder.buildNode(graph);
				built_nodes.push_back({
						{this->x(), this->y()},
						node
						});
				raw_built_nodes.push_back(node);
			}

			for(auto& node : built_nodes) {
				std::size_t edge_count = std::min(
						this->num_edge(), built_nodes.size()-1
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
					/*
					 * Function object that returns a random x coordinate.
					 */
					std::function<double()> x;
					/*
					 * Function object that returns a random y coordinate.
					 */
					std::function<double()> y;

					static random::UniformRealDistribution<double> default_xy_dist;

				public:
					/**
					 * DistributedClusteredGraphBuilder constructor.
					 *
					 * The RandomGraphBuilder::distributed_rd random generator is
					 * used. It can be seeded with RandomGraphBuilder::seed() or
					 * fpmas::seed().
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 * @tparam X_Dist x coordinates distribution (must satisfy
					 * RandomNumberDistribution)
					 * @tparam Y_Dist y coordinates distribution (must satisfy
					 * RandomNumberDistribution)
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
					template<typename EdgeDist, typename X_Dist, typename Y_Dist>
						DistributedClusteredGraphBuilder(
								random::DistributedGenerator<>& generator,
								EdgeDist& edge_distribution,
								X_Dist& x_distribution,
								Y_Dist& y_distribution
								) :
							RandomGraphBuilder(generator, edge_distribution),
							x([&generator, &x_distribution] () {return x_distribution(generator);}),
							y([&generator, &y_distribution] () {return y_distribution(generator);}) {
							}

					/**
					 * DistributedClusteredGraphBuilder constructor.
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 * @tparam X_Dist x coordinates distribution (must satisfy
					 * RandomNumberDistribution)
					 * @tparam Y_Dist y coordinates distribution (must satisfy
					 * RandomNumberDistribution)
					 *
					 * @param edge_distribution random distribution that manages edges
					 * generation. See build()
					 * @param x_distribution random distribution used to assign an
					 * x coordinate to each node
					 * @param y_distribution random distribution used to assign an
					 * y coordinate to each node
					 */
					template<typename EdgeDist, typename X_Dist, typename Y_Dist>
						DistributedClusteredGraphBuilder(
								EdgeDist& edge_distribution,
								X_Dist& x_distribution,
								Y_Dist& y_distribution
								) :
							DistributedClusteredGraphBuilder(
									RandomGraphBuilder::distributed_rd, edge_distribution,
									x_distribution, y_distribution) {
							}


					/**
					 * DistributedClusteredGraphBuilder constructor.
					 *
					 * Built nodes are implicitly and uniformly distributed in a 2D
					 * space.
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 *
					 * @param generator random number generator provided to the
					 * distribution
					 * @param edge_distribution random distribution that manages edges
					 * generation. See build()
					 */
					template<typename EdgeDist>
						DistributedClusteredGraphBuilder(
								random::DistributedGenerator<>& generator,
								EdgeDist& edge_distribution
								) :
							DistributedClusteredGraphBuilder(
									generator, edge_distribution,
									default_xy_dist, default_xy_dist){
							}

					/**
					 * DistributedClusteredGraphBuilder constructor.
					 *
					 * Built nodes are implicitly and uniformly distributed in a 2D
					 * space.
					 *
					 * @tparam EdgeDist edge count distribution (must satisfy
					 * _RandomNumberDistribution_)
					 *
					 * @param edge_distribution random distribution that manages edges
					 * generation. See build()
					 */
					template<typename EdgeDist>
						DistributedClusteredGraphBuilder(
								EdgeDist& edge_distribution
								) :
							DistributedClusteredGraphBuilder(
									RandomGraphBuilder::distributed_rd, edge_distribution,
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
			while(node_builder.localNodeCount() != 0) {
				auto* node = node_builder.buildNode(graph);
				built_nodes.push_back({
						{this->x(), this->y()},
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
						this->num_edge(), built_nodes_buffer.size()-1
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

namespace fpmas { namespace io { namespace datapack {
	/**
	 * fpmas::graph::detail::LocalizedNodeView ObjectPack serializer
	 *
	 * | Serialization Scheme ||||
	 * |----------------------||||
	 * | Point::x | Point::y | DistributedId | int (location rank) |
	 */
	template<typename T>
		struct Serializer<graph::detail::LocalizedNodeView<T>> {

			/**
			 * Returns the buffer size required to serialize a
			 * LocalizedNodeView into `p`.
			 */
			static std::size_t size(
					const ObjectPack& p, const graph::detail::LocalizedNodeView<T>&) {
				return 2*p.size<double>() + p.size<DistributedId>() + p.size<int>();
			}

			/**
			 * Serializes `node` to `pack`.
			 *
			 * @param pack destination pack
			 * @param node node view to serialize
			 */
			static void to_datapack(
					ObjectPack& pack,
					const graph::detail::LocalizedNodeView<T>& node) {
				pack.put(node.p.x);
				pack.put(node.p.y);
				pack.put(node.node_id);
				pack.put(node.location);
			}

			/**
			 * Unserializes a LocalizedNodeView from `pack`.
			 *
			 * @param pack source pack
			 * @return deserialized node view
			 */
			static graph::detail::LocalizedNodeView<T> from_datapack(const ObjectPack& pack) {
				// Call order guaranteed, DO NOT CALL gets FROM THE CONSTRUCTOR
				double x = pack.get<double>();
				double y = pack.get<double>();
				DistributedId id = pack.get<DistributedId>();
				int location = pack.get<int>();
				return {{x, y}, id, location};
			}

		};
}}}
#endif
