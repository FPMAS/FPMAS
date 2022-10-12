#ifndef FPMAS_RANDOM_LB_H
#define FPMAS_RANDOM_LB_H

/** \file src/fpmas/graph/random_load_balancing.h
 * RandomLoadBalancing implementation.
 */

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
			public api::graph::LoadBalancing<T>,
			public api::graph::FixedVerticesLoadBalancing<T> {
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
					: generator(comm), random_rank(0, comm.getSize()-1) {
					}

				/**
				 * \copydoc fpmas::api::graph::LoadBalancing::balance(NodeMap<T>)
				 *
				 * Implements fpmas::api::graph::LoadBalancing
				 */
				PartitionMap balance(api::graph::NodeMap<T> nodes) override;

				/**
				 * Randomly assigns each node to a process.
				 *
				 * Implements fpmas::api::graph::LoadBalancing
				 */
				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMode) override;

				/**
				 * \copydoc fpmas::api::graph::FixedVerticesLoadBalancing::balance(NodeMap<T>, PartitionMap)
				 *
				 * Implements fpmas::api::graph::FixedVerticesLoadBalancing
				 */
				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMap fixed_vertices) override;

				/**
				 * Randomly assigns each node to a process, preserving the
				 * location of `fixed_vertices`.
				 *
				 * Implements fpmas::api::graph::FixedVerticesLoadBalancing
				 */
				PartitionMap balance(
						api::graph::NodeMap<T> nodes,
						api::graph::PartitionMap fixed_vertices,
						api::graph::PartitionMode) override;

		};

	template<typename T>
		PartitionMap RandomLoadBalancing<T>::balance(api::graph::NodeMap<T> nodes) {
			return balance(nodes, api::graph::PARTITION);
		}

	template<typename T>
		PartitionMap RandomLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes, api::graph::PartitionMode partition_mode) {
			return balance(nodes, {}, partition_mode);
		}

	template<typename T>
		PartitionMap RandomLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes,
				api::graph::PartitionMap fixed_vertices) {
			return balance(nodes, fixed_vertices, api::graph::PARTITION);
		}

	template<typename T>
		PartitionMap RandomLoadBalancing<T>::balance(
				api::graph::NodeMap<T> nodes,
				api::graph::PartitionMap fixed_vertices,
				api::graph::PartitionMode) {
			PartitionMap partition = fixed_vertices;
			for(auto node : nodes) {
				partition.insert({node.first, random_rank(generator)});
			}
			return partition;
		}
}}
#endif
