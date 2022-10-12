#ifndef FPMAS_CELL_LB_H
#define FPMAS_CELL_LB_H

/** \file src/fpmas/model/spatial/cell_load_balancing.h
 * CellLoadBalancing implementation.
 */

#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/api/model/spatial/spatial_model.h"

namespace fpmas { namespace model {

	/**
	 * A load balancing algorithm for \SpatialModels.
	 *
	 * A regular \LoadBalancing algorithm is applied **only** to the \Cell
	 * network. Each \SpatialAgent is then assigned to the same process as its
	 * LOCATION \Cell.
	 *
	 * This can significantly reduce the \LoadBalancing algorithm execution time
	 * compared to an application to the global graph, while still providing an
	 * efficient partitioning.
	 *
	 * When the \LoadBalancing algorithm is applied, the weight of each \Cell is
	 * incremented with the weight of all \SpatialAgents located within it in
	 * order to take into account the agent distribution.
	 *
	 * The usage of this algorithm is recommend when the Cell network is dynamic
	 * or the agent distribution is not uniform. Otherwise, the
	 * StaticCellLoadBalancing is much more efficient in most of the case.
	 *
	 * @see StaticCellLoadBalancing
	 */
	class CellLoadBalancing : public api::graph::LoadBalancing<api::model::AgentPtr> {
		private:
			api::communication::MpiCommunicator& comm;
			api::graph::LoadBalancing<api::model::AgentPtr>& cell_lb;

		public:
			/**
			 * CellLoadBalancing constructor.
			 *
			 * @param comm MPI communicator
			 * @param cell_lb LoadBalancing algorithm used to partition the
			 * cell network
			 */
			CellLoadBalancing(
					api::communication::MpiCommunicator& comm,
					api::graph::LoadBalancing<api::model::AgentPtr>& cell_lb)
				: comm(comm), cell_lb(cell_lb) {
				}


			/**
			 * \copydoc api::graph::LoadBalancing::balance(NodeMap<T>)
			 */
			api::graph::PartitionMap balance(
					api::graph::NodeMap<api::model::AgentPtr> nodes
					) override;

			/**
			 * Builds a partition from the input nodes.
			 *
			 * First, a \LoadBalancing algorithm is applied only to `nodes`
			 * containing an fpmas::api::model::Cell instance. The specified
			 * `partition_mode` is passed to this \LoadBalancing algorithm.
			 *
			 * Then, each \SpatialAgent is assigned to the same process as its
			 * LOCATION \Cell.
			 *
			 * Other nodes are ignored.
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

	/**
	 * A load balancing algorithm for \SpatialModels.
	 *
	 * Just as the CellLoadBalancing algorithm, an existing LoadBalancing
	 * algorithm is applied only to the cell network, and agents are
	 * systematically assigned to the same process as their location cell.
	 *
	 * But this time, the existing algorithm is only applied in PARTITION mode,
	 * so that the algorithm only assign agent to the right process in
	 * REPARTITION mode.
	 *
	 * The usage of this algorithm, much faster than the CellLoadBalancing, is
	 * recommend when the Cell network is static and the agent distribution is
	 * uniform.
	 *
	 * @see CellLoadBalancing
	 */
	class StaticCellLoadBalancing : public api::graph::LoadBalancing<api::model::AgentPtr> {
		private:
			api::communication::MpiCommunicator& comm;
			api::graph::LoadBalancing<api::model::AgentPtr>& cell_lb;

		public:
			/**
			 * CellLoadBalancing constructor.
			 *
			 * @param comm MPI communicator
			 * @param cell_lb LoadBalancing algorithm used to partition the
			 * cell network
			 */
			StaticCellLoadBalancing(
					api::communication::MpiCommunicator& comm,
					api::graph::LoadBalancing<api::model::AgentPtr>& cell_lb)
				: comm(comm), cell_lb(cell_lb) {
				}


			/**
			 * \copydoc api::graph::LoadBalancing::balance(NodeMap<T>)
			 */
			api::graph::PartitionMap balance(
					api::graph::NodeMap<api::model::AgentPtr> nodes
					) override;

			/**
			 * Builds a partition from the input nodes.
			 *
			 * In PARTITION mode, a \LoadBalancing algorithm in PARTITION mode
			 * only to `nodes` containing an fpmas::api::model::Cell instance.
			 * 
			 * Then, in any mode, each \SpatialAgent is assigned to the same
			 * process as its LOCATION \Cell.
			 *
			 * Other nodes are ignored.
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
