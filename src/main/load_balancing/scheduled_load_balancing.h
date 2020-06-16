#ifndef SCHEDULED_LOAD_BALANCING_H
#define SCHEDULED_LOAD_BALANCING_H

#include "api/load_balancing/load_balancing.h"
#include "api/scheduler/scheduler.h"
#include "api/runtime/runtime.h"

namespace FPMAS::load_balancing {
	template<typename T>
		class ScheduledLoadBalancing : public api::load_balancing::LoadBalancing<T> {
			private:
				api::load_balancing::FixedVerticesLoadBalancing<T>& fixed_vertices_lb;
				api::scheduler::Scheduler& scheduler;
				api::runtime::Runtime& runtime;

			public:
				using typename api::load_balancing::LoadBalancing<T>::PartitionMap;
				using typename api::load_balancing::LoadBalancing<T>::ConstNodeMap;

				ScheduledLoadBalancing<T>(
						api::load_balancing::FixedVerticesLoadBalancing<T>& fixed_vertices_lb,
						api::scheduler::Scheduler& scheduler,
						api::runtime::Runtime& runtime
						) : fixed_vertices_lb(fixed_vertices_lb), scheduler(scheduler), runtime(runtime) {}

				PartitionMap balance(ConstNodeMap nodes) override;

		};

	template<typename T>
		typename ScheduledLoadBalancing<T>::PartitionMap
		ScheduledLoadBalancing<T>::balance(ConstNodeMap nodes) {
			scheduler::Epoch epoch;
			scheduler.build(runtime.currentDate() + 1, epoch);
			ConstNodeMap node_map;
			PartitionMap fixed_nodes;
			PartitionMap partition;
			for(const api::scheduler::Job* job : epoch) {
				for(api::scheduler::Task* task : *job) {
					if(api::scheduler::NodeTask<T>* node_task = dynamic_cast<api::scheduler::NodeTask<T>*>(task)) {
						auto node = node_task->node();
						node_map[node->getId()] = node;
					}
				}
				partition = fixed_vertices_lb.balance(node_map, fixed_nodes);
				fixed_nodes = partition;
			};
			partition = fixed_vertices_lb.balance(nodes, fixed_nodes);
			return partition;
		}
}
#endif
