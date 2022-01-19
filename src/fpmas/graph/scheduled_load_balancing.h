#ifndef FPMAS_SCHEDULED_LOAD_BALANCING_H
#define FPMAS_SCHEDULED_LOAD_BALANCING_H

/** \file src/fpmas/graph/scheduled_load_balancing.h
 * ScheduledLoadBalancing implementation.
 */

#include "fpmas/api/graph/graph.h"
#include "fpmas/api/runtime/runtime.h"
#include "fpmas/scheduler/scheduler.h"
#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/communication/communication.h"

#include <set>

namespace fpmas { namespace graph {
	using api::graph::PartitionMap;
	using api::graph::NodeMap;

	/**
	 * api::graph::LoadBalancing implementation that takes nodes' task
	 * scheduling into account.
	 */
	template<typename T>
		class ScheduledLoadBalancing : public api::graph::LoadBalancing<T> {
			private:
				api::graph::FixedVerticesLoadBalancing<T>& fixed_vertices_lb;
				api::scheduler::Scheduler& scheduler;
				api::runtime::Runtime& runtime;
				
			public:
				/**
				 * ScheduledLoadBalancing constructor.
				 *
				 * @param fixed_vertices_lb fixed vertices load balancing
				 * algorithm
				 * @param scheduler current scheduler (used to access scheduled
				 * tasks associated to nodes)
				 * @param runtime current runtime (used to access the current
				 * api::runtime::Runtime::currentDate())
				 */
				ScheduledLoadBalancing(
						api::graph::FixedVerticesLoadBalancing<T>& fixed_vertices_lb,
						api::scheduler::Scheduler& scheduler,
						api::runtime::Runtime& runtime
						) : fixed_vertices_lb(fixed_vertices_lb), scheduler(scheduler), runtime(runtime) {}

				/**
				 * \copydoc fpmas::api::graph::LoadBalancing::balance(NodeMap<T>)
				 *
				 * Implements fpmas::api::graph::LoadBalancing
				 */
				PartitionMap balance(api::graph::NodeMap<T> nodes) override;

				/**
				 * \copydoc fpmas::api::graph::FixedVerticesLoadBalancing::balance(NodeMap<T>, PartitionMap)
				 *
				 * Implements fpmas::api::graph::LoadBalancing
				 */
				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMode partition_mode) override;

		};

	template<typename T>
		PartitionMap ScheduledLoadBalancing<T>::balance(api::graph::NodeMap<T> nodes) {
			return balance(nodes, api::graph::PARTITION);
		}

	template<typename T>
		PartitionMap ScheduledLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes, api::graph::PartitionMode partition_mode) {
			scheduler::Epoch epoch;
			scheduler.build(runtime.currentDate() + 1, epoch);
			// Global node map
			NodeMap<T> node_map;
			// Nodes currently fixed
			PartitionMap fixed_nodes;
			// Current partition
			PartitionMap partition;

			for(const api::scheduler::Job* job : epoch) {
				for(api::scheduler::Task* task : *job) {
					if(api::scheduler::NodeTask<T>* node_task = dynamic_cast<api::scheduler::NodeTask<T>*>(task)) {
						// Node is bound to a Task that will be executed in
						// job. Only LOCAL nodes are considered in this
						// process, since DISTANT nodes are never executed.
						auto node = node_task->node();
						node_map[node->getId()] = node;
					}
				}
				// Partitions only the nodes that will be executed in this job,
				// fixing nodes that will be executed in previous jobs (that
				// are already partitionned). Notice that fixed_nodes is
				// included in node_map.
				partition = fixed_vertices_lb.balance(node_map, fixed_nodes, partition_mode);
				// Fixes nodes currently partitionned
				fixed_nodes = partition;
			};
			// Finally, partition all other nodes, that are not executed within
			// this epoch, or that are not bound to any Task.
			partition = fixed_vertices_lb.balance(nodes, fixed_nodes, partition_mode);
			return partition;
		}
}}
#endif
