#ifndef FPMAS_STATIC_LB_H
#define FPMAS_STATIC_LB_H

#include "fpmas/api/graph/load_balancing.h"

/** \file src/fpmas/graph/static_load_balancing.h
 * StaticLoadBalancing implementation.
 */

namespace fpmas { namespace graph {
	using api::graph::NodeMap;
	using api::graph::PartitionMap;

	/**
	 * A \LoadBalancing algorithm implementation that takes as input an existing
	 * LoadBalancing algorithm and applies it only in
	 * fpmas::api::graph::PARTITION mode. Nothing is ever done in
	 * fpmas::api::graph::REPARTITION mode, even if the existing algorithm
	 * supports this mode.
	 */
	template<typename T>
		class StaticLoadBalancing : public api::graph::LoadBalancing<T> {
			private:
				api::graph::LoadBalancing<T>& lb_algorithm;
			public:
				/**
				 * StaticLoadBalancing constructor.
				 *
				 * @param lb_algorithm Existing \LoadBalancing algorithm,
				 * applied as a static load balancing algorithm.
				 */
				StaticLoadBalancing(api::graph::LoadBalancing<T>& lb_algorithm)
					: lb_algorithm(lb_algorithm) {
					}
				/**
				 * \copydoc fpmas::api::graph::LoadBalancing::balance(NodeMap<T>)
				 */
				PartitionMap balance(api::graph::NodeMap<T> nodes) override;

				/**
				 * If mode is PARTITION, applies the existing \LoadBalancing
				 * algorithm to the nodes. Else, does nothing.
				 *
				 * @param nodes local nodes to balance
				 * @param mode partitioning strategy
				 * @return balanced partition map
				 */
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
