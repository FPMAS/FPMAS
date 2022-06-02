#ifndef FPMAS_SMALL_WORLD_GRAPH_BUILDER_H
#define FPMAS_SMALL_WORLD_GRAPH_BUILDER_H

#include "fpmas/graph/distributed_node.h"
#include "fpmas/random/distribution.h"
#include "fpmas/random/random.h"
#include "ring_graph_builder.h"
#include "random_graph_builder.h"

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
	 * 1. Build `N` nodes, and connect them in a CYCLE as specicfied in the
	 * RingGraphBuilder. `K/2` is used as a parameter for the RingGraphBuilder,
	 * so that each node as `K` outgoing neighbors in total (`K/2` on each side).
	 * 2. Consider the outgoing edges of each node, and rewire it with a
	 * probability `p/2` to a node chosen among all the possible nodes. If the
	 * current node is selected or a link from the current node to the chosen
	 * node already exists, nothing is done. The outgoing edge rewiring
	 * procedure consists in changing the target node of the edge.
	 * 3. Do exactly the same but with incoming edges, changing source nodes of
	 * incoming edges with a probability `p/2`. Edges considered are only
	 * incoming edges from the original CYCLE: incoming edges resulting of
	 * rewiring from the previous step are not considered, so that edges cannot
	 * be rewired twice.
	 *
	 * # Explanations
	 * Each edge is considered twice (one time as incoming edge, and one time as
	 * outgoing) with a probability `p/2`, so each edge has finally a probability
	 * `p` to be rewired. It si required to do so to prevent the following
	 * issue: if edges where considered only once in the outgoing rewiring
	 * process (with a probability `p`), all nodes would necessarily have
	 * exactly `K` outgoing neighbors at the end of the process, what introduces
	 * a bias.
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

			std::set<DistributedId> rewired_edges;
			{
				// First pass: rewire out edges
				std::vector<api::graph::DistributedEdge<T>*> edges_to_rewire;
				random::UniformRealDistribution<float> rd_float(0, 1);
				for(auto node : local_nodes)
					for(auto edge : node->getOutgoingEdges(layer))
						if(rd_float(RandomGraphBuilder::distributed_rd) < p/2)
							edges_to_rewire.push_back(edge);

				std::vector<std::pair<DistributedId, int>> random_nodes(edges_to_rewire.size());
				{
					auto it = random_nodes.begin();
					std::vector<std::vector<DistributedId>> random_choices =
						random::distributed_choices(
								graph.getMpiCommunicator(),
								local_ids,
								edges_to_rewire.size());
					for(std::size_t i = 0; i < random_choices.size(); i++)
						for(auto& item : random_choices[i])
							*it++ = {item, i};
				}

				for(std::size_t i = 0; i < edges_to_rewire.size(); i++) {
					if(random_nodes[i].first != edges_to_rewire[i]->getSourceNode()->getId()) {
						auto current_source_out_edges = 
							edges_to_rewire[i]->getSourceNode()->getOutgoingEdges(layer);
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
								new_target = graph.insertDistant(new DistributedNode<T>(random_nodes[i].first, T()));
								new_target->setLocation(random_nodes[i].second);
							}

							auto new_edge = graph.link(edges_to_rewire[i]->getSourceNode(), new_target, layer);
							rewired_edges.insert(new_edge->getId());
							graph.unlink(edges_to_rewire[i]);
						}
					}
				}
			}
			graph.synchronize();
			{
				// Second pass: rewire in edges
				std::vector<api::graph::DistributedEdge<T>*> edges_to_rewire;
				random::UniformRealDistribution<float> rd_float(0, 1);
				for(auto node : local_nodes)
					for(auto edge : node->getIncomingEdges(layer))
						// Do not consider incoming edges resulting from the
						// previous out rewiring procedure
						if(rewired_edges.find(edge->getId()) == rewired_edges.end())
							if(rd_float(RandomGraphBuilder::distributed_rd) < p/2)
								edges_to_rewire.push_back(edge);

				std::vector<std::pair<DistributedId, int>> random_nodes(edges_to_rewire.size());
				{
					auto it = random_nodes.begin();
					std::vector<std::vector<DistributedId>> random_choices =
						random::distributed_choices(
								graph.getMpiCommunicator(),
								local_ids,
								edges_to_rewire.size());
					for(std::size_t i = 0; i < random_choices.size(); i++)
						for(auto& item : random_choices[i])
							*it++ = {item, i};
				}

				for(std::size_t i = 0; i < edges_to_rewire.size(); i++) {
					if(random_nodes[i].first != edges_to_rewire[i]->getTargetNode()->getId()) {
						auto current_target_in_edges = 
							edges_to_rewire[i]->getTargetNode()->getIncomingEdges(layer);
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
								new_source = graph.insertDistant(new DistributedNode<T>(random_nodes[i].first, T()));
								new_source->setLocation(random_nodes[i].second);
							}

							graph.link(new_source, edges_to_rewire[i]->getTargetNode(), layer);
							graph.unlink(edges_to_rewire[i]);
						}
					}
				}
			}
			graph.synchronize();

			return local_nodes;
		}
}}

#endif
