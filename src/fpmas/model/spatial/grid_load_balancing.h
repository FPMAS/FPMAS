#ifndef FPMAS_GRID_LB_H
#define FPMAS_GRID_LB_H

/** \file src/fpmas/model/spatial/grid_load_balancing.h
 * GridLoadBalancing implementation.
 */

#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/api/model/spatial/grid.h"

#include <list>

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	/**
	 * Utility class used to describe the shape of a discrete grid.
	 *
	 * A grid is described by its origin point and an extent until which the
	 * grid extents.
	 *
	 * The `origin` is included in the grid, while the `extent` is
	 * **not**
	 *
	 * For example, GridDimensions({0, 0}, {10, 10}) describes a grid
	 * where the bottom left corner is at (0, 0), and the top right
	 * corner is at (9, 9).
	 */
	class GridDimensions {
		private:
			DiscretePoint origin;
			DiscretePoint extent;

		public:
			/**
			 * GridDimensions constructor.
			 *
			 * @param origin origin point of the grid
			 * @param extent point to until which the grid extents
			 */
			GridDimensions(DiscretePoint origin, DiscretePoint extent)
				: origin(origin), extent(extent) {
				}
			/**
			 * Default GridDimensions constructor.
			 */
			GridDimensions() {}

			/**
			 * Origin of the grid.
			 */
			DiscretePoint getOrigin() const {return origin;}
			/**
			 * Sets the origin of the grid.
			 */
			void setOrigin(const DiscretePoint& point) {this->origin = point;}

			/**
			 * Extent of the grid.
			 */
			DiscretePoint getExtent() const {return extent;}
			/**
			 * Sets the extent of the grid.
			 */
			void setExtent(const DiscretePoint& point) {this->extent = point;}

			/**
			 * Returns the width of the grid, i.e. the count of cells contained
			 * in the grid width.
			 */
			DiscreteCoordinate width() const {
				return extent.x - origin.x;
			}
			/**
			 * Returns the height of the grid, i.e. the count of cells contained
			 * in the grid height.
			 */
			DiscreteCoordinate height() const {
				return extent.y - origin.y;
			}
	};

	/**
	 * When a graph is distributed using a grid-based load-balancing algorithm,
	 * a portion of the 2D discrete space is associated to each process.
	 *
	 * This generic interface is used to map each DiscretePoint of a grid to a
	 * process, in order to perform GridLoadBalancing.
	 */
	class GridProcessMapping {
		public:
			/**
			 * Returns the rank of the process that owns the specified point,
			 * according to the current GridProcessMapping implementation.
			 *
			 * In other terms, this is the rank of the process where the
			 * \GridCell located at `point` and all \SpatialAgents located in it
			 * will be \LOCAL.
			 */
			virtual int process(DiscretePoint point) const = 0;

			/**
			 * Returns the dimensions of the local grid part associated to the
			 * specified process.
			 */
			virtual GridDimensions gridDimensions(int process) const = 0;

			virtual ~GridProcessMapping() {}
	};

	/**
	 * Fast and simple algorithm to partition a 2D grid.
	 *
	 * The grid is recursively split in both directions of the space in order
	 * to minimize sizes of frontiers between processes. When the count of
	 * processes in a power of 2, the algorithm produces a "classical" grid
	 * decomposition as performed by RepastHPC.
	 *
	 * The algorithm can handle any process count, including those that are not
	 * a power of 2.
	 *
	 * The grid can be of any size, without any restriction on width or height.
	 */
	class TreeProcessMapping : public GridProcessMapping {
		private:
			enum NodeType {
				HORIZONTAL_FRONTIER,
				VERTICAL_FRONTIER,
				LEAF
			};

			std::vector<GridDimensions> process_grid_dimensions;

			struct Node {
				// Coordinates of the bottom left corner of the zone
				// represented by this node
				DiscretePoint origin;
				// Width of the node represented by this node
				DiscreteCoordinate width;
				// Height of the node represented by this node
				DiscreteCoordinate height;

				NodeType node_type;
				// When the node is a frontier, the value between the first and
				// second child. Depending on the orientation of the frontier,
				// this is an x or y coordinate.
				DiscreteCoordinate value;

				// Children of the node (empty if the node is a LEAF, exactly two
				// children otherwise)
				std::vector<Node*> childs;

				// When the node is a LEAF, process that owns the zone
				// represented by this node
				int process = -1;
			};
			Node* root;

			/*
			 * Splits the first node of the list in two nodes, that become the
			 * children of the node.
			 *
			 * The zone is split vertically if node->height > node->width,
			 * horizontally otherwise, in order to minimize the size of
			 * frontiers.
			 *
			 * By construction, the leafs are ordered so that largest zones are
			 * at the beginning of the list, so its consistent to split those
			 * nodes first. Indeed, split nodes are always pushed back in the
			 * list, while the first element of leafs is popped. Moreover, at
			 * the beginning of the algorithm, the only element in `leafs` is
			 * `root`, that represents the whole grid.
			 *
			 * Also note that this procedure increases the leafs size by 1: 2
			 * children are added at the end of the list, while the split node
			 * is removed from the beginning of the list.
			 */
			void split_node(std::list<Node*>& leafs);

			/*
			 * Recursively deletes all nodes of the tree.
			 */
			void delete_node(Node* node);

			/*
			 * Recursively finds the LEAF node that contains `point`, and
			 * returns the rank of the process associated to this node.
			 */
			int process(DiscretePoint point, Node* node) const;

		public:
			/**
			 * Default TreeProcessMapping constructor, **for internal use
			 * only**.
			 *
			 * To prevent unexpected crashes, process() always returns 0.
			 */
			TreeProcessMapping();

			/**
			 * TreeProcessMapping constructor.
			 *
			 * @param width width of the grid
			 * @param height height of the grid
			 * @param comm MPI communicator
			 */
			TreeProcessMapping(
				DiscreteCoordinate width, DiscreteCoordinate height,
				api::communication::MpiCommunicator& comm
				);

			TreeProcessMapping(const TreeProcessMapping&) = delete;
			TreeProcessMapping& operator=(const TreeProcessMapping&) = delete;

			int process(DiscretePoint point) const override;

			GridDimensions gridDimensions(int process) const override;

			~TreeProcessMapping();
	};

	/**
	 * 1D GridProcessMapping implementation, that produces horizontal or
	 * vertical strip shaped partitions.
	 *
	 * The grid is divided in a single direction, in order to minimize the size
	 * of frontiers.
	 *
	 * So, if the width of the grid is greater than its height, the grid is
	 * divided using vertical borders, otherwise horizontal borders are used.
	 *
	 * This algorithm produces well balanced partitions, but sizes of the
	 * frontiers are generally much bigger that ones produced by the
	 * TreeProcessMapping, so this last method is preferred.
	 */
	class StripProcessMapping : public GridProcessMapping {
		private:
			/**
			 * Described how the grid is divided.
			 */
			enum Mode {
				/**
				 * HORIZONTAL partitioning = vertical borders
				 */
				HORIZONTAL,
				/**
				 * VERTICAL partitioning = horizontal borders
				 */
				VERTICAL
			};
			Mode mode;
			std::map<DiscreteCoordinate, int> process_bounds;
			std::vector<GridDimensions> process_grid_dimensions;

		public:
			/**
			 * GridProcessMapping constructor.
			 *
			 * @param width global grid width
			 * @param height global grid height
			 * @param comm MPI communicator
			 */
			StripProcessMapping(
				DiscreteCoordinate width, DiscreteCoordinate height,
				api::communication::MpiCommunicator& comm
				);

			/**
			 * Returns the rank of the process to which the `point` is
			 * associated.
			 *
			 * The result is undefined if the specified point is not contained
			 * in the grid of size `width x height`, so `point.x` must be in
			 * `[0, width-1]` and `point.y` must be in `[0, height-1]`.
			 *
			 * @param point discrete point of the grid
			 * @return process associated to `point`
			 */
			int process(DiscretePoint point) const override;

			GridDimensions gridDimensions(int process) const override;
	};
	/**
	 * A grid based load balancing algorithm.
	 *
	 * This algorithm only applies to \Agents of the graph that implement
	 * api::model::GridCell or api::model::GridAgent (or, more specifically,
	 * api::model::GridAgentBase).
	 *
	 * However notice that several implementations, i.e. distinct types
	 * extending each interface, can simultaneously exist in the graph, and the
	 * algorithm is indifferently applied to all of them.
	 *
	 * If \Agents that do **not** implement one of those interface are contained
	 * in the graph, the algorithm can still be applied, but those \Agents are
	 * ignored: they will always be associated to the process that already
	 * owned them when the algorithm is applied.
	 *
	 * The partitioning of the environment is performed using a
	 * GridProcessMapping, that first optimally associates each
	 * api::model::GridCell to a process. Then, each api::model::GridAgent is
	 * associated to the process that owns its `locationCell()`, so that each
	 * api::model::GridCell and all api::model::GridAgents located on that cell
	 * are all \LOCAL from their process.
	 *
	 * The frequency at which the algorithm is executed is relatively
	 * important. Indeed, the DistributedMoveAlgorithm still allows agents to
	 * move past the "borders" associated to each process. This means that when
	 * an agent moves, is can be temporarily associated to a \DISTANT location,
	 * until the GridLoadBalancing is applied again.
	 */
	class GridLoadBalancing : public api::graph::LoadBalancing<api::model::AgentPtr> {
		private:
			TreeProcessMapping default_process_mapping;
			const GridProcessMapping& grid_process_mapping;

		public:
			/**
			 * GridLoadBalancing constructor.
			 *
			 * By default, a TreeProcessMapping instance is used to map
			 * GridCells to processes.
			 *
			 * @param width global grid width
			 * @param height global grid height
			 * @param comm MPI communicator
			 */
			GridLoadBalancing(
				DiscreteCoordinate width, DiscreteCoordinate height,
				api::communication::MpiCommunicator& comm
				);

			/**
			 * GridLoadBalancing constructor.
			 *
			 * The specified GridProcessMapping is used instead of the default
			 * TreeProcessMapping.
			 */
			GridLoadBalancing(const GridProcessMapping& grid_process_mapping);

			
			/**
			 * \copydoc api::graph::LoadBalancing::balance(NodeMap<T>)
			 */
			api::graph::PartitionMap balance(
					api::graph::NodeMap<api::model::AgentPtr> nodes
					) override;

			/**
			 * Builds a partition according to the grid based load balancing
			 * algorithm from the input nodes.
			 *
			 * Nodes that don't inherit from api::model::GridCell or
			 * api::model::GridAgentBase are ignored and not contained in the
			 * final partition, so that their state is unchanged when the
			 * partition is given back to the \DistributedGraph.
			 *
			 * @param nodes nodes on which the algorithm is applied
			 * @param partition_mode partitioning strategy
			 * @return grid based partition
			 */
			api::graph::PartitionMap balance(
					api::graph::NodeMap<api::model::AgentPtr> nodes,
					api::graph::PartitionMode partition_mode
					) override;
	};

}}
#endif
