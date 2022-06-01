#ifndef FPMAS_GRAPH_ANALYSIS_H
#define FPMAS_GRAPH_ANALYSIS_H

/** \file src/fpmas/graph/analysis.h
 * Contains utilies to perform graph analysis.
 */

#include "fpmas/api/graph/distributed_graph.h"
#include "fpmas/communication/communication.h"

#include <set>

namespace fpmas { namespace graph {
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

}}

#endif
