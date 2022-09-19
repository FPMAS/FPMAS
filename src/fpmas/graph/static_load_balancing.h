#ifndef FPMAS_STATIC_LB_H
#define FPMAS_STATIC_LB_H

#include "fpmas/api/graph/load_balancing.h"

namespace fpmas { namespace graph {
	using api::graph::NodeMap;
	using api::graph::PartitionMap;

	template<typename T>
		class StaticLoadBalancing : public api::graph::LoadBalancing<T> {
			private:
				api::graph::LoadBalancing<T>& lb_algorithm;
			public:
				StaticLoadBalancing(api::graph::LoadBalancing<T>& lb_algorithm)
					: lb_algorithm(lb_algorithm) {
					}

				PartitionMap balance(api::graph::NodeMap<T> nodes) override;

				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMode mode) override;
		};

	template<typename T>
		PartitionMap StaticLoadBalancing<T>::balance(api::graph::NodeMap<T> nodes) {
			return balance(nodes, api::graph::PARTITION);
		}

	template<typename T>
		PartitionMap StaticLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes,
				api::graph::PartitionMode mode) {
			switch(mode) {
				case api::graph::PARTITION:
					return lb_algorithm.balance(nodes, api::graph::PARTITION);
				default:
					return {};
			}
		}
}}
#endif
