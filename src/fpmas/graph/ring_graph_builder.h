#ifndef FPMAS_RING_GRAPH_BUILDER_H
#define FPMAS_RING_GRAPH_BUILDER_H

/** \file src/fpmas/graph/ring_graph_builder.h
 * Algorithms to build ring graphs.
 */

#include "fpmas/api/graph/graph_builder.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/distributed_node.h"
#include "fpmas/utils/functional.h"

#include <list>

namespace fpmas { namespace graph {
	/**
	 * Ring graph types.
	 */
	enum RingType {
		/**
		 * A ring graph linked only in one direction, forming a directed loop.
		 */
		LOOP,
		/**
		 * A ring graph linked in both directions.
		 */
		CYCLE
	};

	/**
	 * Implements an api::graph::DistributedGraphBuilder algorithm to build
	 * ring shaped graphs.
	 *
	 * The ring can be linked in one direction (LOOP) or in both directions
	 * (CYCLE).
	 *
	 * The number of closest neighbors to which each node is connected can be
	 * controlled with the parameter `k`.
	 *
	 * ## Examples
	 *
	 * N_processes |LOOP, k=1 || LOOP, k=2 ||
	 * :----------:|----------||-----------||
	 * 1 | ![LOOP, k=1](loop_k1_n1.png) || ![LOOP, k=2](loop_k2_n1.png) ||
	 * 4 | ![LOOP, k=1, 0](loop_k1_n4_0.png) | ![LOOP, k=1, 1](loop_k1_n4_1.png) | ![LOOP, k=2, 0](loop_k2_n4_0.png) | ![LOOP, k=2, 1](loop_k2_n4_1.png)
	 * ^ | ![LOOP, k=1, 3](loop_k1_n4_3.png) | ![LOOP, k=1, 2](loop_k1_n4_2.png) | ![LOOP, k=2, 3](loop_k2_n4_3.png) | ![LOOP, k=2, 2](loop_k2_n4_2.png)
	 *
	 * N_processes |CYCLE, k=1 || CYCLE, k=2 ||
	 * :----------:|----------||-----------||
	 * 1 | ![CYCLE, k=1](cycle_k1_n1.png) || ![CYCLE, k=2](cycle_k2_n1.png) ||
	 * 4 | ![CYCLE, k=1, 0](cycle_k1_n4_0.png) | ![CYCLE, k=1, 1](cycle_k1_n4_1.png) | ![CYCLE, k=2, 0](cycle_k2_n4_0.png) | ![CYCLE, k=2, 1](cycle_k2_n4_1.png)
	 * ^ | ![CYCLE, k=1, 3](cycle_k1_n4_3.png) | ![CYCLE, k=1, 2](cycle_k1_n4_2.png) | ![CYCLE, k=2, 3](cycle_k2_n4_3.png) | ![CYCLE, k=2, 2](cycle_k2_n4_2.png)
	 */
	template<typename T>
		class RingGraphBuilder : public api::graph::DistributedGraphBuilder<T> {
			private:
				std::size_t k;
				RingType ring_type;

				template<typename LocalNodeIterator>
				std::vector<api::graph::DistributedNode<T>*> build_distant_nodes(
						// Graph in which distant nodes are inserted
						api::graph::DistributedGraph<T>& graph,
						// Used to build distant nodes
						api::graph::DistributedNodeBuilder<T>& node_builder,
						// Number of nodes available on each processes
						const std::vector<std::size_t>& node_counts,
						// An iterator to the first nodes that should be send
						// to each processes, depending on the current loop
						// direction.
						LocalNodeIterator local_nodes_iterator,
						// returns the rank of the process at `distance` from
						// the current process, depending on the current loop
						// direction.
						std::function<int (int /*level*/)> next_rank
						);
				template<typename NodeIterator>
					void link_loop(
							api::graph::DistributedGraph<T>& graph,
							NodeIterator local_node_iterator,
							std::size_t local_node_count,
							NodeIterator link_node_iterator,
							api::graph::LayerId layer
							);
			public:
				/**
				 * RingGraphBuilder constructor.
				 *
				 * @param k the connected nearest neighbors count of each node
				 * **in each direction**. So, if `k=2` in a LOOP, each node has
				 * 2 outgoing neighbors. If `k=2` in a CYCLE, each node has 2
				 * outgoing neighbors in each direction, so 4 in total.
				 * @param ring_type ring type: CYCLE or LOOP
				 */
				RingGraphBuilder(std::size_t k, RingType ring_type=CYCLE)
					: k(k), ring_type(ring_type) {
				}

				std::vector<api::graph::DistributedNode<T>*> build(
						api::graph::DistributedNodeBuilder<T>& node_builder,
						api::graph::LayerId layer,
						api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T> template <typename LocalNodeIterator>
		std::vector<api::graph::DistributedNode<T>*> RingGraphBuilder<T>
		::build_distant_nodes(
				api::graph::DistributedGraph<T>& graph,
				api::graph::DistributedNodeBuilder<T>& node_builder,
				const std::vector<std::size_t>& node_counts,
				LocalNodeIterator local_nodes_iterator,
				std::function<int (int)> next_rank
				) {
			// MPI set up
			fpmas::communication::TypedMpi<std::size_t> int_mpi(
					graph.getMpiCommunicator()
					);
			fpmas::communication::TypedMpi<std::list<DistributedId>> list_id_mpi(
					graph.getMpiCommunicator()
					);

			std::unordered_map<int, std::size_t> node_count_requests;
			std::unordered_map<int, std::list<DistributedId>> ids_requests;

			/*
			 * Feeds the node_count_requests to know how many distant nodes
			 * must be imported from each process.
			 * The objective is to found enough nodes so that all the local
			 * nodes on the current process can be linked to k nodes.
			 */
			// Count of found nodes
			std::size_t count = 0;
			// Distance from the inspected process
			int level = 1;
			// List of ranks from which nodes will be imported, ordered from
			// the closest to the furthest. Notice that the current rank might
			// be contained at the end of the list. (this is for example
			// required for N=4 and k=3 on 2 processes)
			std::vector<int> ranks;
			while(count < k) {
				// Inspect the process at a distance `level` from the current
				// process. Depending on the current loop orientation,
				// `_next_rank` might be before or after the current process.
				int _next_rank = next_rank(level);
				// Computes the number of nodes that will be imported from
				// next_rank
				std::size_t node_count = std::min(
						// Imports only the exact node count required, so that
						// at the end of the while loop count == k
						node_counts[_next_rank], k-count
						);
				node_count_requests[_next_rank] = node_count;
				count+=node_count;
				level++;
				ranks.push_back(_next_rank);
			}

			node_count_requests = int_mpi.allToAll(node_count_requests);

			// Respond to other processes requests
			for(auto item : node_count_requests) {
				// Depending on the current loop orientation, `it` will
				// correspond to the begin or end of local nodes
				auto it = local_nodes_iterator;

				// In any case, the `item.second` nodes away from
				// `local_nodes_iterator` must be sent to the requesting
				// process.
				std::list<DistributedId>& ids = ids_requests[item.first];
				for(std::size_t i = 0; i < item.second; i++)
					// it++ goes forward or backward, depending on the
					// specified iterator
					ids.push_back((*it++)->getId());
			}

			ids_requests = list_id_mpi.allToAll(ids_requests);

			// Builds all the required distant nodes
			std::vector<api::graph::DistributedNode<T>*> built_nodes;
			for(auto rank : ranks)
				for(auto node_id : ids_requests[rank])
					// If a node corresponding to node_id was already in the
					// graph, nothing is done but the returned node is the one
					// actually contained in the graph, so built_nodes stays
					// consistent
					built_nodes.push_back(
							node_builder.buildDistantNode(node_id, rank, graph));

			return built_nodes;
		}
				
	template<typename T>
		template<typename NodeIterator>
		void RingGraphBuilder<T>::link_loop(
				api::graph::DistributedGraph<T>& graph,
				NodeIterator local_node_iterator,
				std::size_t local_node_count,
				NodeIterator link_node_iterator,
				api::graph::LayerId layer
				) {
			// Create links between nodes to build a loop
			//
			// At this point, the link_node list contains exactly k distant
			// nodes in addition to the local_node list. Depending of the
			// current orientation of the built loop, the k additional distant
			// nodes might be at the beginning or at the end of the link_node
			// list, but this is not important here: the algorithm starts from
			// the provided `local_node_iterator`, and links each of the
			// `local_node_count` local node to its k nearest neighbors in the
			// loop. The last local nodes will notably be linked to the last k
			// distant nodes of the link_node list.
			NodeIterator local_node = local_node_iterator;
			for(std::size_t n = 0; n < local_node_count; n++) {
				NodeIterator link_node = link_node_iterator;
				// Puts the `link_node` iterator at the same position as
				// `local_node` in the local node list
				std::advance(link_node, n);
				// Links the local_node to the k next nodes in the link node
				// list
				for(std::size_t i = 0; i < k; i++) {
					// Selects the next link node
					link_node++;
					// Links the local node to the link node
					graph.link(*local_node, *link_node, layer);
				}
				local_node++;
			}

			graph.synchronizationMode().getSyncLinker().synchronize();
		}

	template<typename T>
		std::vector<api::graph::DistributedNode<T>*> RingGraphBuilder<T>::build(
				api::graph::DistributedNodeBuilder<T>& node_builder,
				api::graph::LayerId layer,
				api::graph::DistributedGraph<T>& graph) {
			// Builds local nodes
			std::list<api::graph::DistributedNode<T>*> local_nodes;
			while(node_builder.localNodeCount() > 0)
				local_nodes.push_back(node_builder.buildNode(graph));

			// MPI related set up
			int rank = graph.getMpiCommunicator().getRank();
			int size = graph.getMpiCommunicator().getSize();

			fpmas::communication::TypedMpi<std::size_t> int_mpi(
					graph.getMpiCommunicator());

			// Gathers the total node counts on each processes
			std::vector<std::size_t> node_counts = int_mpi.allGather(local_nodes.size());

			{
				/*
				 * Forward loop
				 */

				std::list<api::graph::DistributedNode<T>*> link_nodes = local_nodes;

				auto distant_nodes = build_distant_nodes(
						graph, node_builder,
						node_counts,
						// The BEGINNING of the local nodes will be sent to the
						// PREVIOUS processes
						local_nodes.begin(),
						[rank, size] (int level) -> int {
							// Nodes are requested to processes AFTER the
							// current process
							return (rank+level)%size;
						}
						);

				// The BEGIN nodes are imported from the NEXT processes: they
				// correspond to the nodes that will be link to the END nodes
				// of the current process, so they are added at the end of the
				// link_nodes list.
				for(auto node : distant_nodes)
					link_nodes.push_back(node);

				// Links forward loop: the local node list is crossed from
				// begin to end
				link_loop(
						graph,
						local_nodes.begin(), local_nodes.size(),
						link_nodes.begin(), layer
						);
			} if(ring_type == CYCLE) {
				/*
				 * Backward loop
				 */
				std::list<api::graph::DistributedNode<T>*> link_nodes = local_nodes;

				auto distant_nodes = build_distant_nodes(
						graph, node_builder,
						node_counts,
						// The ENDING of the local nodes will be sent to the
						// NEXT processes
						local_nodes.rbegin(),
						[rank, size] (int current_level) -> int {
							// Nodes are requested to processes BEFORE the
							// current process
							return (rank+size-current_level)%size;
						}
						);
				
				// The END nodes are imported from the PREVIOUS processes: they
				// correspond to the nodes that will be linked to the BEGIN
				// nodes of the current process, so they are added at beginning
				// of the link_nodes list.
				for(auto node : distant_nodes)
					link_nodes.push_front(node);

				// Links backward loop: the local node list is crossed from end
				// to begin
				link_loop(
						graph,
						local_nodes.rbegin(), local_nodes.size(),
						link_nodes.rbegin(), layer
						);
			}

			return {local_nodes.begin(), local_nodes.end()};
		}
}}
#endif
