#ifndef FPMAS_GRAPH_ANALYSIS_H
#define FPMAS_GRAPH_ANALYSIS_H

/** \file src/fpmas/graph/analysis.h
 * Contains utilies to perform graph analysis.
 */

#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/communication/communication.h"
#include "fpmas/utils/functional.h"
#include <set>
#include <queue>

namespace fpmas { namespace graph {
	/**
	 * Counts the total number of nodes contained in the distributed graph.
	 *
	 * This method is synchronous: it can't return before it as been initiated
	 * by all processes in graph.getMpiCommunicator(), since it requires
	 * collective communications.
	 *
	 * @param graph Distributed graph instance
	 * @return total node count within the graph
	 */
	template<typename T> std::size_t node_count(api::graph::DistributedGraph<T>& graph) {
		communication::TypedMpi<std::size_t> int_mpi(graph.getMpiCommunicator());
		return communication::all_reduce(
				int_mpi, graph.getLocationManager().getLocalNodes().size());
	}
	/**
	 * Counts the total number of edges contained in the distributed graph.
	 *
	 * This method is synchronous: it can't return before it as been initiated
	 * by all processes in graph.getMpiCommunicator(), since it requires
	 * collective communications.
	 *
	 * @param graph Distributed graph instance
	 * @return total edge count within the graph
	 */
	template<typename T>
		std::size_t edge_count(api::graph::DistributedGraph<T>& graph) {
			std::size_t local_count = 0;
			for(auto node : graph.getLocationManager().getLocalNodes())
				local_count += node.second->getOutgoingEdges().size();

			communication::TypedMpi<std::size_t> int_mpi(graph.getMpiCommunicator());
			return communication::all_reduce(int_mpi, local_count);
		}

	/**
	 * Utility method used to retrieve the ids of outgoing neighbors of \DISTANT
	 * nodes in the graph.
	 *
	 * This can be used to build a representation of neighbors of \DISTANT
	 * nodes, since such information is not supposed to be preserved by the
	 * DistributedGraph distribution process (only edges between \LOCAL and
	 * \DISTANT nodes are preserved).
	 *
	 * Only edges on the specified layer are considered.
	 *
	 * This method is synchronous: it can't return before it as been initiated
	 * by all processes in graph.getMpiCommunicator(), since it requires
	 * collective communications.
	 *
	 * @param graph distributed graph input
	 * @param layer graph layer
	 * @return a map containing an entry for each \DISTANT node, in the form
	 * {node_id: {list of ids of outgoing neighbors}}
	 */
	template<typename T>
		std::unordered_map<DistributedId, std::vector<DistributedId>>
		distant_nodes_outgoing_neighbors(
				fpmas::api::graph::DistributedGraph<T>& graph,
				fpmas::api::graph::LayerId layer) {
			// MPI instance used to exchange DistributedIds requests
			communication::TypedMpi<std::vector<DistributedId>> requests_mpi(
					graph.getMpiCommunicator());
			// Builds a request for each \DISTANT node in the graph
			std::unordered_map<int, std::vector<DistributedId>> requests;
			for(auto node : graph.getLocationManager().getDistantNodes())
				requests[node.second->location()].push_back(node.first);
			requests = requests_mpi.allToAll(requests);

			// MPI instance used to send request results. Results are received
			// as a vector of pairs {requested node ids, {list of ids of
			// outgoing neighbors}}.
			communication::TypedMpi<std::vector<std::pair<DistributedId, std::vector<DistributedId>>>> results_mpi(graph.getMpiCommunicator());
			std::unordered_map<int, std::vector<std::pair<DistributedId, std::vector<DistributedId>>>> results;
			for(auto request : requests) {
				for(auto node_id : request.second) {
					// Safely gets neighbors of a LOCAL node
					auto edges = graph.getNode(node_id)->getOutgoingEdges(layer);
					// Stores neighbors ids in a list
					std::vector<DistributedId> neighbors(edges.size());
					auto it = neighbors.begin();
					for(auto edge : edges)
						*it++ = edge->getTargetNode()->getId();
					// Save the {node_id, {neighbors}} pair, ready to be
					// transmitted
					results[request.first].push_back({node_id, neighbors});
				}
			}
			// Exchange results
			results = results_mpi.allToAll(results);

			// Gathers results from all processes
			std::unordered_map<DistributedId, std::vector<DistributedId>>
				distant_nodes_neighbors;
			for(auto item : results)
				for(auto neighbors : item.second)
					distant_nodes_neighbors[neighbors.first] = neighbors.second;
			// Returns results for all DISTANT nodes on the current process
			return distant_nodes_neighbors;
		}

	/**
	 * Computes the [local clustering
	 * coefficient](https://en.wikipedia.org/wiki/Clustering_coefficient#Local_clustering_coefficient)
	 * of the specified distributed graph, as specified by Duncan J. Watts and
	 * Steven H. Strogatz to characterize small world networks. Only the edges
	 * on the specified layer are considered, so that a clustering coefficient
	 * can be computed independently for each layer.
	 *
	 * The purpose of the measure for a single node is to evaluate how its
	 * neighbors are connected together, computing the proportion of existing
	 * edges between them. The algorithm returns the average value for all
	 * nodes in the graph.
	 *
	 * This method is synchronous: it can't return before it as been initiated
	 * by all processes in graph.getMpiCommunicator(), since it requires
	 * collective communications.
	 *
	 * This distributed implementation guarantees scalability, since the amount
	 * of additionnal data imported is only proportionnal to the count of
	 * \DISTANT nodes in the graph.
	 *
	 * @param graph distributed graph input
	 * @param layer layer on which the coefficient is computed
	 * @return local clustering coefficient
	 */
	template<typename T>
		double clustering_coefficient(
				fpmas::api::graph::DistributedGraph<T>& graph,
				fpmas::api::graph::LayerId layer) {
			double c = 0;
			auto distant_nodes_neighbors = distant_nodes_outgoing_neighbors(graph, layer);

			for(auto node : graph.getLocationManager().getLocalNodes()) {
				std::size_t n = 0;

				// Neighbors of `node`= the set of incoming AND outgoing
				// neighbors, without duplicate. The purpose of the algorithm is
				// then to count edges between those neighbors.
				std::set<DistributedId> neighbors;
				for(auto edge : node.second->getIncomingEdges(layer)) {
					neighbors.insert(edge->getSourceNode()->getId());
				}
				for(auto edge : node.second->getOutgoingEdges(layer)) {
					neighbors.insert(edge->getTargetNode()->getId());
				}
				// Do not consider self edges
				neighbors.erase(node.first);

				for(auto node_id : neighbors) {
					// The objective is to count edges between neighbor with
					// others in the neighbors list. Only OUTGOING edges are
					// considered: since each neighbors is considered, this
					// guarantees that each edge is counted exactly once.
					auto neighbor = graph.getNode(node_id);
					if(neighbor->state() == fpmas::api::graph::LOCAL) {
						// Edges can be retrieved locally
						for(auto edge : neighbor->getOutgoingEdges(layer)) {
							auto id = edge->getTargetNode()->getId();
							if(
									// Do not consider edge to the considered node
									id != node.first
									// Only edges from a node's neighbor to an other
									&& neighbors.find(id) != neighbors.end()) {
								++n;
							}
						}
					} else {
						// The neighbor is \DISTANT, but its neighbors can be
						// retrieved with the distant_nodes_neighbors ids map.
						for(auto id : distant_nodes_neighbors[node_id])
							if(id != node.first && neighbors.find(id) != neighbors.end()) {
								++n;
							}
					}
				}
				std::size_t k = neighbors.size();
				if(k > 1)
					c += (double) n / (k*(k-1));
			}
			communication::TypedMpi<std::size_t> int_mpi(graph.getMpiCommunicator());
			communication::TypedMpi<double> double_mpi(graph.getMpiCommunicator());
			c = communication::all_reduce(double_mpi, c);
			std::size_t total_node_count =
				communication::all_reduce(int_mpi, graph.getLocationManager().getLocalNodes().size());
			if(total_node_count > 0)
				return c/total_node_count;
			else
				return 0;
		}

	template<typename T>
		void __explore_distances(
				api::graph::DistributedId source,
				std::queue<std::pair<api::graph::DistributedNode<T>*, float>>& node_queue,
				api::graph::LayerId layer,
				std::unordered_map<DistributedId, std::unordered_map<DistributedId, float>>& distances,
				std::unordered_map<
				int,
				std::unordered_map<DistributedId, std::unordered_map<DistributedId, float>>
				>& next_processes) {
			while(!node_queue.empty()) {
				auto current_node = node_queue.front();
				node_queue.pop();
				if(current_node.first->state() == api::graph::LOCAL) {
					if(current_node.second < distances[current_node.first->getId()][source]) {
						distances[current_node.first->getId()][source] = current_node.second;
						for(auto edge : current_node.first->getOutgoingEdges(layer))
							node_queue.push({edge->getTargetNode(), current_node.second+1});
					}
				} else {
					// Map with all requests for node from any source
					auto& current_node_requests = next_processes
						[current_node.first->location()][current_node.first->getId()];
					// Request for the current node from the current source
					auto current_source_request = current_node_requests.find(source);
					if(current_source_request == current_node_requests.end())
						// No request from the current source, create new one
						current_node_requests[source] = current_node.second;
					else if(current_node.second < current_node_requests[source])
						// Else a request already exist, but replace it only if it
						// can produce a shortest path
						current_node_requests[source] = current_node.second;
				}
			}
		}


	/**
	 * Returns a vector containing the ids of \LOCAL nodes within the
	 * distributed graph.
	 *
	 * @param graph distributed graph
	 * @return ids of \LOCAL nodes
	 */
	template<typename T>
		std::vector<DistributedId> local_node_ids(
				fpmas::api::graph::DistributedGraph<T>& graph) {
			std::vector<DistributedId> ids(
					graph.getLocationManager().getLocalNodes().size());
			auto it = ids.begin();
			for(auto& item : graph.getLocationManager().getLocalNodes()) {
				*it = item.first;
				++it;
			}
			return ids;
		}

	/**
	 * Computes and return a `distance_map` such that for all local nodes in the
	 * graph and all specified source nodes the entry `distance_map[node
	 * id][source id]` contains the length of the shortest path from `source` to
	 * `node` on the specified layer. If no path is found, the distance is set
	 * to `std::numeric_limits<float>::infinity()`.
	 *
	 * This method is synchronous: it can't return before it as been initiated
	 * by all processes in graph.getMpiCommunicator(), since it requires
	 * collective communications.
	 *
	 * @param graph distributed graph
	 * @param layer layer of edges to consider
	 * @param source_nodes ids of source node from which distances should be
	 * computed
	 */
	template<typename T>
		std::unordered_map<DistributedId, std::unordered_map<DistributedId, float>>
		shortest_path_lengths(
				fpmas::api::graph::DistributedGraph<T>& graph,
				fpmas::api::graph::LayerId layer,
				const std::vector<DistributedId>& source_nodes
				) {
			fpmas::communication::TypedMpi<std::vector<DistributedId>> vector_id_mpi(
					graph.getMpiCommunicator());
			// Map local node: (source node: distance)
			std::unordered_map<DistributedId, std::unordered_map<DistributedId, float>>
				distances;
			std::vector<DistributedId> all_source_nodes = communication::all_reduce(
					vector_id_mpi, source_nodes, fpmas::utils::Concat());
			api::graph::NodeMap<T> local_nodes
				= graph.getLocationManager().getLocalNodes();
			for(auto node : local_nodes)
				for(auto id : all_source_nodes)
					distances[node.first][id] = std::numeric_limits<float>::infinity();

			std::unordered_map<int,
				std::unordered_map<DistributedId, std::unordered_map<DistributedId, float>>
					> next_processes;
			fpmas::communication::TypedMpi<
				std::unordered_map<DistributedId, std::unordered_map<DistributedId, float>>>
				next_mpi(graph.getMpiCommunicator());
			for(auto source : all_source_nodes) {
				auto source_node = local_nodes.find(source);
				if(source_node != local_nodes.end()) {
					std::queue<std::pair<api::graph::DistributedNode<T>*, float>>
						node_queue;
					node_queue.push({source_node->second, 0});
					__explore_distances(source, node_queue, layer, distances, next_processes);
				}
			}

			communication::TypedMpi<bool> bool_mpi(graph.getMpiCommunicator());
			auto and_fct = 
				[](const bool& b1, const bool& b2) {return b1 && b2;};
			bool end = communication::all_reduce(
					bool_mpi, next_processes.size() == 0, and_fct
					);
			while(!end) {
				auto requests = next_mpi.allToAll(next_processes);
				next_processes.clear();
				for(auto proc : requests) {
					for(auto node : proc.second)
						for(auto source : node.second) {
							// Re-launch exploration from local node
							std::queue<std::pair<api::graph::DistributedNode<T>*, float>> node_queue;
							node_queue.push({graph.getNode(node.first), source.second});
							__explore_distances(
									source.first, node_queue, layer,
									distances, next_processes
									);
						}
				}
				end = communication::all_reduce(
						bool_mpi, next_processes.size() == 0, and_fct
						);
			}

			return distances;
		}

	/**
	 * Computes the average shortest path length from sources to targets
	 * reachable on the specified layer.
	 *
	 * When a target is not reachable, an infinite characteristic path length
	 * should mathematically be returned, but in order to get finite and
	 * meaningful values we prefer to ignore those pairs of nodes so that the
	 * characteristic path length is actually computed on the largest weakly
	 * connected component of the graph.
	 *
	 * The source_nodes and layer parameters can be used to compute the
	 * characteristic path length only on a subgraph.
	 *
	 * For example, in the case of a \SpatialModel, the following snippet allows
	 * to compute the characteristic path length of the \Cell network, ignoring
	 * \SpatialAgents contained in the graph:
	 * ```cpp
	 *	float L = fpmas::graph::characteristic_path_length(
	 *		model.getModel().graph(), fpmas::api::model::CELL_SUCCESSOR,
	 *		fpmas::model::local_agent_ids(model.cellGroup()));
	 * ```
	 *
	 * @param graph distributed graph
	 * @param layer layer of edges to consider
	 * @param source_nodes ids of source node from which distances should be
	 * computed
	 */
	template<typename T>
		float characteristic_path_length(
				fpmas::api::graph::DistributedGraph<T>& graph,
				fpmas::api::graph::LayerId layer,
				const std::vector<DistributedId>& source_nodes
				) {
			auto distances = shortest_path_lengths(graph, layer, source_nodes);

			communication::TypedMpi<float> distance_mpi(graph.getMpiCommunicator());
			communication::TypedMpi<std::size_t> int_mpi(graph.getMpiCommunicator());
			float local_sum = 0;
			// Number of node pairs between which a path actually exists
			std::size_t num_pairs = 0;
			for(auto& source : distances)
				for(auto& distance : source.second) {
					if(source.first != distance.first &&
							distance.second < std::numeric_limits<float>::infinity()) {
						local_sum+=distance.second;
						num_pairs++;
					}
				}
			float total_sum = communication::all_reduce(distance_mpi, local_sum);
			num_pairs = communication::all_reduce(int_mpi, num_pairs);
			return total_sum / num_pairs;
		}
}}

#endif
