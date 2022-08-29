#ifndef FPMAS_SMALL_WORLD_GRAPH_BUILDER_H
#define FPMAS_SMALL_WORLD_GRAPH_BUILDER_H

#include "fpmas/graph/distributed_node.h"
#include "fpmas/random/distribution.h"
#include "fpmas/random/random.h"
#include "ring_graph_builder.h"
#include "random_graph_builder.h"
#include <fpmas/api/graph/location_state.h>
#include <fpmas/communication/communication.h>

namespace fpmas { namespace graph {

	/**
	 * A distributed procedure to build [Small World
	 * graphs](https://en.wikipedia.org/wiki/Small-world_network), inspired from the
	 * work of Duncan J. Watts and Steven H. Strogatz. The [original
	 * procedure](https://en.wikipedia.org/wiki/Watts%E2%80%93Strogatz_model) as
	 * been adapted to support directed graphs.
	 *
	 * We consider a node count `N`, a mean output degree `K` (supposed to be
	 * even), and a probability `p`.
	 * 1. Build `N` nodes, and connect them in a CYCLE as specified in the
	 * RingGraphBuilder. `K/2` is used as a parameter for the RingGraphBuilder,
	 * so that each node as `K` outgoing neighbors in total (`K/2` on each side).
	 * 2. Consider each edge, and select it for rewiring with a probability `p`.
	 * If selected, chose if target or source will be rewired with a probability
	 * `0.5`.
	 * 3. Rewires all edges that need a new target. The new target is chosen
	 * randomly among all the nodes. Nothing is done if the randomly selected
	 * new target corresponds to the current source, of if an edge already
	 * exists from the current source to the selected target.
	 * 4. Synchronization barrier
	 * 5. Analog procedure for all edges that need a new source.
	 * 
	 * # Explanations
	 * - It is necessary to randomly chose which side of each edge to rewire, to
	 *   prevent all nodes to have exactly `K` outgoing neighbors at the end of
	 *   the process.
	 * - The synchronization barrier prevents concurrency issues. For example,
	 *   during step 3, it is not possible for any process other that the
	 *   current one to build a new outgoing edge from the current source, since
	 *   only new targets are selected, so that it is safe to check for existing
	 *   outgoing edges.
	 *
	 * # Example
	 * | | n=16, k=4, p=0.1 ||
	 * :----------:|-----------||
	 * Global | ![n=16, k=4](small_world_n16_k4_p01.png) ||
	 * Distributed | ![n=16, k=4, 0](small_world_n16_k4_p01_0.png) | ![n=16, k=4, 1](small_world_n16_k4_p01_1.png) |
	 * ^ | ![n=16, k=4, 2](small_world_n16_k4_p01_2.png) | ![n=16, k=4, 3](small_world_n16_k4_p01_3.png) |
	
	 */
	template<typename T>
		class SmallWorldGraphBuilder : public api::graph::DistributedGraphBuilder<T> {
			private:
				float p;
				RingGraphBuilder<T> ring_graph_builder;

			public:
				/**
				 * SmallWorldGraphBuilder constructor.
				 *
				 * Notice that the node count is deduced from the count of nodes
				 * available in the node builder passed to the build() method.
				 *
				 * In order to respect the Small World assumptions, it is
				 * supposed that `k` is even and `N >> k >> ln(N)`. With `p=0`,
				 * the graph will be perfectly regular, with `p=1`, it will be
				 * completely random. In practice, low values of `p` (like
				 * `p=0.1`) are enough to expose the small world properties
				 * without falling in a completely random graph. The intuitive
				 * reason for that is that the idea of random rewiring is to
				 * create some shortcuts across the original ring to reduce
				 * distance from initially far nodes, and only a few are
				 * required to globally reduce lengths of paths in the graph,
				 * while preserving a high clustering coefficient.
				 *
				 * The build process uses the
				 * RandomGraphBuilder::distributed_rd, so the process can be
				 * seeded with RandomGraphBuilder::seed() or fpmas::seed().
				 *
				 * @param p probability of rewriring each edge
				 * @param k initial output degree of each node (assumed to be
				 * even)
				 */
				SmallWorldGraphBuilder(float p, std::size_t k)
					// k/2 = number of neihbors on each side of the node in the
					// ring_graph_builder process
					: p(p), ring_graph_builder(k/2, CYCLE) {
					}

				std::vector<api::graph::DistributedNode<T>*> build(
						api::graph::DistributedNodeBuilder<T>& node_builder,
						api::graph::LayerId layer,
						api::graph::DistributedGraph<T>& graph) override;
		};

	template<typename T>
		std::vector<api::graph::DistributedNode<T>*> SmallWorldGraphBuilder<T>::build(
						api::graph::DistributedNodeBuilder<T>& node_builder,
						api::graph::LayerId layer,
						api::graph::DistributedGraph<T>& graph) {
			std::vector<api::graph::DistributedNode<T>*> local_nodes
				= ring_graph_builder.build(node_builder, layer, graph);
			const auto& graph_nodes = graph.getNodes();
			std::vector<DistributedId> local_ids(local_nodes.size());
			for(std::size_t i = 0; i < local_ids.size(); i++)
				local_ids[i] = local_nodes[i]->getId();

			// First pass: rewire out edges
			std::vector<api::graph::DistributedEdge<T>*> pre_in_edges_to_rewire;
			std::vector<api::graph::DistributedEdge<T>*> out_edges_to_rewire;
			random::UniformRealDistribution<float> rd_float(0, 1);
			random::BernoulliDistribution rd_bool(0.5);
			for(auto node : local_nodes)
				for(auto edge : node->getOutgoingEdges(layer))
					if(rd_float(RandomGraphBuilder::distributed_rd) < p) {
						if(rd_bool(RandomGraphBuilder::distributed_rd))
							out_edges_to_rewire.push_back(edge);
						else
							pre_in_edges_to_rewire.push_back(edge);
					}
			std::unordered_map<int, std::vector<DistributedId>> in_edges_rewiring_requests;
			{
				// First pass: rewire out edges
				std::vector<std::pair<DistributedId, int>> random_nodes(out_edges_to_rewire.size());
				{
					auto it = random_nodes.begin();
					std::vector<std::vector<DistributedId>> random_choices =
						random::distributed_choices(
								graph.getMpiCommunicator(),
								local_ids,
								out_edges_to_rewire.size());
					for(std::size_t i = 0; i < random_choices.size(); i++)
						for(auto& item : random_choices[i])
							*it++ = {item, i};
				}

				for(std::size_t i = 0; i < out_edges_to_rewire.size(); i++) {
					if(random_nodes[i].first != out_edges_to_rewire[i]->getSourceNode()->getId()) {
						assert(out_edges_to_rewire[i]->getSourceNode()->state()
								==  api::graph::LOCAL);
						auto current_source_out_edges = 
							out_edges_to_rewire[i]->getSourceNode()->getOutgoingEdges(layer);
						// Ensures the edge from source to random node is not built
						// yet
						if(std::find_if(
									current_source_out_edges.begin(), current_source_out_edges.end(),
									[&random_nodes, &i] (api::graph::DistributedEdge<T>* edge) {
									return edge->getTargetNode()->getId() == random_nodes[i].first;
									}) == current_source_out_edges.end()) {
							api::graph::DistributedNode<T>* new_target;
							auto existing_node = graph_nodes.find(random_nodes[i].first);
							if(existing_node != graph_nodes.end()) {
								new_target = existing_node->second;
							} else {
								new_target = node_builder.buildDistantNode(
										random_nodes[i].first, random_nodes[i].second,
										graph
										);
							}

							graph.link(out_edges_to_rewire[i]->getSourceNode(), new_target, layer);
							graph.unlink(out_edges_to_rewire[i]);
						}
					}
				}
				out_edges_to_rewire.clear();
			}
			graph.synchronize();

			std::vector<api::graph::DistributedEdge<T>*> in_edges_to_rewire;
			for(auto edge : pre_in_edges_to_rewire)
				if(edge->getTargetNode()->state() != api::graph::LOCAL)
					in_edges_rewiring_requests[edge->getTargetNode()->location()]
						.push_back(edge->getId());
				else
					in_edges_to_rewire.push_back(edge);
			// Frees memory
			pre_in_edges_to_rewire.clear();

			fpmas::communication::TypedMpi<std::vector<DistributedId>> id_mpi(
					graph.getMpiCommunicator());
			in_edges_rewiring_requests = id_mpi.allToAll(in_edges_rewiring_requests);
			for(auto item : in_edges_rewiring_requests)
				for(auto edge_id : item.second)
					in_edges_to_rewire.push_back(graph.getEdge(edge_id));
			// Frees memory
			in_edges_rewiring_requests.clear();
			{
				// Second pass: rewire in edges
				std::vector<std::pair<DistributedId, int>> random_nodes(in_edges_to_rewire.size());
				{
					auto it = random_nodes.begin();
					std::vector<std::vector<DistributedId>> random_choices =
						random::distributed_choices(
								graph.getMpiCommunicator(),
								local_ids,
								in_edges_to_rewire.size());
					for(std::size_t i = 0; i < random_choices.size(); i++)
						for(auto& item : random_choices[i])
							*it++ = {item, i};
				}

				for(std::size_t i = 0; i < in_edges_to_rewire.size(); i++) {
					if(random_nodes[i].first != in_edges_to_rewire[i]->getTargetNode()->getId()) {
						assert(in_edges_to_rewire[i]->getTargetNode()->state()
								==  api::graph::LOCAL);
						auto current_target_in_edges = 
							in_edges_to_rewire[i]->getTargetNode()->getIncomingEdges(layer);
						// Ensures the edge from random node to target is not built
						// yet
						if(std::find_if(
									current_target_in_edges.begin(), current_target_in_edges.end(),
									[&random_nodes, &i] (api::graph::DistributedEdge<T>* edge) {
									return edge->getSourceNode()->getId() == random_nodes[i].first;
									}) == current_target_in_edges.end()) {
							api::graph::DistributedNode<T>* new_source;
							auto existing_node = graph_nodes.find(random_nodes[i].first);
							if(existing_node != graph_nodes.end()) {
								new_source = existing_node->second;
							} else {
								new_source = node_builder.buildDistantNode(
										random_nodes[i].first, random_nodes[i].second,
										graph
										);
							}

							graph.link(new_source, in_edges_to_rewire[i]->getTargetNode(), layer);
							graph.unlink(in_edges_to_rewire[i]);
						}
					}
				}
			}
			graph.synchronize();

			return local_nodes;
		}
}}

#endif
