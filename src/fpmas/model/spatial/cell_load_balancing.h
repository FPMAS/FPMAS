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
}}
#endif
