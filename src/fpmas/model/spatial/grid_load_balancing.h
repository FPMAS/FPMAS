#ifndef FPMAS_GRID_LB_H
#define FPMAS_GRID_LB_H

#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/api/model/spatial/grid.h"

namespace fpmas { namespace model {
	using api::model::DiscretePoint;
	using api::model::DiscreteCoordinate;

	/**
	 * Maps a DiscretePoint to a specific process.
	 *
	 * When a graph is distributed using a grid-based load-balancing algorithm,
	 * a contiguous portion of the 2D discrete space is associated to each
	 * process. This class associates a process to each 2D coordinate,
	 * according to the grid dimensions, in order to minimize the size of the
	 * frontiers (i.e. the DISTANT node counts) between processes.
	 *
	 * So, if the width of the grid is greater than its height, the grid is
	 * divided using vertical borders, otherwise horizontal borders are used.
	 */
	class GridProcessMapping {
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

		public:
			/**
			 * GridProcessMapping constructor.
			 *
			 * @param width global grid width
			 * @param height global grid height
			 * @param comm MPI communicator
			 */
			GridProcessMapping(
				DiscreteCoordinate width, DiscreteCoordinate height,
				api::communication::MpiCommunicator& comm
				);

			/**
			 * Returns the rank of the process to which the `point` is
			 * associated.
			 *
			 * In other terms, this is the process that owns the `GridCell`
			 * located at point, i.e. where this `GridCell` is \LOCAL.
			 *
			 * The result is undefined if the specified point is not contain in
			 * the grid of size `width x height`, so `point.x` must be in `[0,
			 * width-1]` and `point.y` must be in `[0, height-1]`.
			 *
			 * @param point discrete point of the grid
			 * @return process associated to `point`
			 */
			int process(DiscretePoint point);

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
			GridProcessMapping grid_process_mapping;

		public:
			/**
			 * GridLoadBalancing constructor.
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
			 * Builds a partition according to the grid based load balancing
			 * algorithm from the input nodes.
			 *
			 * Nodes that don't inherit from api::model::GridCell or
			 * api::model::GridAgentBase are ignored and not contained in the
			 * final partition, so that their state is unchanged when the
			 * partition is given back to the \DistributedGraph.
			 *
			 * @param nodes nodes on which the algorithm is applied
			 * @return grid based partition
			 */
			api::graph::PartitionMap balance(
					api::graph::NodeMap<api::model::AgentPtr> nodes
					) override;
	};

}}
#endif
