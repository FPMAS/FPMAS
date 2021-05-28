#ifndef FPMAS_RANDOM_LB_H
#define FPMAS_RANDOM_LB_H

#include "fpmas/api/communication/communication.h"
#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/random/random.h"

namespace fpmas { namespace graph {
	using api::graph::NodeMap;
	using api::graph::PartitionMap;

	/**
	 * Implements a random fpmas::api::graph::LoadBalancing and
	 * fpmas::api::graph::FixedVerticesLoadBalancing algorithm.
	 *
	 * Nodes are randomly and uniformly assigned to a random process, while
	 * assigning `fixed_vertices` to the corresponding processes when
	 * specified.
	 */
	template<typename T>
		class RandomLoadBalancing :
			public fpmas::api::graph::LoadBalancing<T>,
			public fpmas::api::graph::FixedVerticesLoadBalancing<T> {
			private:
				fpmas::random::DistributedGenerator<> generator;
				fpmas::random::UniformIntDistribution<int> random_rank;


			public:
				/**
				 * RandomLoadBalancing constructor.
				 *
				 * @param comm MPI communicator
				 */
				RandomLoadBalancing(api::communication::MpiCommunicator& comm)
					: random_rank(0, comm.getSize()-1) {
					}

				/**
				 * \copydoc fpmas::api::graph::LoadBalancing::balance()
				 */
				PartitionMap balance(NodeMap<T> nodes) override;
				/**
				 * \copydoc fpmas::api::graph::FixedVerticesLoadBalancing::balance()
				 */
				PartitionMap balance(
						NodeMap<T> nodes,
						PartitionMap fixed_vertices) override;

		};

	template<typename T>
		PartitionMap RandomLoadBalancing<T>::balance(NodeMap<T> nodes) {
			return balance(nodes, {});
		}

	template<typename T>
		PartitionMap RandomLoadBalancing<T>::balance(
				NodeMap<T> nodes,
				PartitionMap fixed_vertices) {
			PartitionMap partition = fixed_vertices;
			for(auto node : nodes) {
				partition.insert({node.first, random_rank(generator)});
			}
			return partition;
		}
}}
#endif
