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
				api::communication::MpiCommunicator& comm;
				struct ConcatSet {
					std::set<DistributedId> operator()(
							const std::set<DistributedId> s1,
							const std::set<DistributedId> s2
							) {
						std::set<DistributedId> s;
						s.insert(s1.begin(), s1.end());
						s.insert(s2.begin(), s2.end());
						return s;
					}
				};

			public:
				/**
				 * @deprecated
				 *
				 * ScheduledLoadBalancing constructor.
				 *
				 * @param fixed_vertices_lb fixed vertices load balancing
				 * algorithm
				 * @param scheduler current scheduler (used to access scheduled
				 * tasks associated to nodes)
				 * @param runtime current runtime (used to access the current
				 * api::runtime::Runtime::currentDate())
				 */
				[[deprecated]]
				ScheduledLoadBalancing<T>(
						api::graph::FixedVerticesLoadBalancing<T>& fixed_vertices_lb,
						api::scheduler::Scheduler& scheduler,
						api::runtime::Runtime& runtime
						) : fixed_vertices_lb(fixed_vertices_lb), scheduler(scheduler), runtime(runtime), comm(communication::WORLD) {}

				/**
				 * ScheduledLoadBalancing constructor.
				 *
				 * @param fixed_vertices_lb fixed vertices load balancing
				 * algorithm
				 * @param scheduler current scheduler (used to access scheduled
				 * tasks associated to nodes)
				 * @param runtime current runtime (used to access the current
				 * api::runtime::Runtime::currentDate())
				 * @param comm MPI communicator
				 */
				ScheduledLoadBalancing<T>(
						api::graph::FixedVerticesLoadBalancing<T>& fixed_vertices_lb,
						api::scheduler::Scheduler& scheduler,
						api::runtime::Runtime& runtime,
						api::communication::MpiCommunicator& comm
						) : fixed_vertices_lb(fixed_vertices_lb), scheduler(scheduler), runtime(runtime), comm(comm) {}


				PartitionMap balance(NodeMap<T> nodes) override;

		};

	template<typename T>
		PartitionMap ScheduledLoadBalancing<T>::balance(NodeMap<T> nodes) {
			scheduler::Epoch epoch;
			scheduler.build(runtime.currentDate() + 1, epoch);
			// Global node map
			NodeMap<T> node_map;
			// Nodes currently fixed
			PartitionMap fixed_nodes;
			// Current partition
			PartitionMap partition;
			communication::TypedMpi<std::set<DistributedId>> mpi(comm);

			for(const api::scheduler::Job* job : epoch) {
				std::vector<api::graph::DistributedNode<T>*> local_job_nodes;
				std::set<DistributedId> job_nodes;
				for(api::scheduler::Task* task : *job) {
					if(api::scheduler::NodeTask<T>* node_task = dynamic_cast<api::scheduler::NodeTask<T>*>(task)) {
						// Node is bound to a Task that will be executed in
						// job. Only LOCAL nodes are considered in this
						// process, since DISTANT nodes are never executed.
						auto node = node_task->node();
						node_map[node->getId()] = node;

						local_job_nodes.push_back(node);
						job_nodes.insert(node->getId());
					}
				}
				// Fetches ids of **all** the nodes that will be executed in
				// the current job
				job_nodes = communication::all_reduce(mpi, job_nodes, ConcatSet());
				// Increments node_map with DISTANT nodes that are also
				// executed in the current job, but on other processes
				for(auto job_node : local_job_nodes) {
					for(auto neighbor : job_node->outNeighbors())
						if(neighbor->state() == api::graph::DISTANT && job_nodes.count(neighbor->getId()) > 0)
							node_map[neighbor->getId()] = neighbor;
					for(auto neighbor : job_node->inNeighbors())
						if(neighbor->state() == api::graph::DISTANT && job_nodes.count(neighbor->getId()) > 0)
							node_map[neighbor->getId()] = neighbor;
				}
				// Partitions only the nodes that will be executed in this job,
				// fixing nodes that will be executed in previous jobs (that
				// are already partitionned). Notice that fixed_nodes is
				// included in node_map.
				partition = fixed_vertices_lb.balance(node_map, fixed_nodes);
				// Fixes nodes currently partitionned
				fixed_nodes = partition;
			};
			// Finally, partition all other nodes, that are not executed within
			// this epoch, or that are not bound to any Task.
			partition = fixed_vertices_lb.balance(nodes, fixed_nodes);
			return partition;
		}
}}
#endif
