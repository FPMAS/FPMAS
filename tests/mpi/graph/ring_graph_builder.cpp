#include "fpmas/graph/ring_graph_builder.h"
#include "fpmas/graph/graph_builder.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"

#include "gmock/gmock.h"

using namespace testing;

class DistributedNodeBuilder : public fpmas::graph::DistributedNodeBuilder<int> {
	public:
		using fpmas::graph::DistributedNodeBuilder<int>::DistributedNodeBuilder;

		fpmas::api::graph::DistributedNode<int>* buildNode(
				fpmas::api::graph::DistributedGraph<int>& graph) override {
			this->local_node_count--;
			return graph.buildNode(0);
		}

		fpmas::api::graph::DistributedNode<int>* buildDistantNode(
				fpmas::api::graph::DistributedId id,
				int location,
				fpmas::api::graph::DistributedGraph<int>& graph) override {
			auto new_node = new fpmas::graph::DistributedNode<int>(id, 0);
			new_node->setLocation(location);
			return graph.insertDistant(new_node);
		}

};

/*
 * Builds a "simple" loop: the k required distant neighbors are all located on
 * the next process.
 */
TEST(RingGraphBuilder, build_loop) {

	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph(
			fpmas::communication::WORLD
			);
	// K = 2
	fpmas::graph::RingGraphBuilder<int> builder(2, fpmas::graph::LOOP);
	// 4 nodes by process, so there is only a need to import 2 nodes from the
	// next process to link the loop
	DistributedNodeBuilder node_builder(
			4*graph.getMpiCommunicator().getSize(),
			graph.getMpiCommunicator()
			);

	builder.build(node_builder, 0, graph);

	for(auto node : graph.getLocationManager().getLocalNodes()) {
		std::set<fpmas::graph::DistributedId> out_neighbors;
		for(auto edge : node.second->getOutgoingEdges(0))
			out_neighbors.insert(edge->getTargetNode()->getId());
		ASSERT_THAT(out_neighbors, SizeIs(2));

		std::set<fpmas::graph::DistributedId> in_neighbors;
		for(auto edge : node.second->getIncomingEdges(0))
			in_neighbors.insert(edge->getSourceNode()->getId());
		ASSERT_THAT(in_neighbors, SizeIs(2));
	}
}

/*
 * Builds a complete loop, that is considered as the limit of the algorithm: in
 * this edge case, all nodes are linked to all other. In consequence, it will
 * be required to import distant nodes from all other processes to build the
 * loop.
 */
TEST(RingGraphBuilder, build_complete_loop) {
	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph(
			fpmas::communication::WORLD
			);

	// K = N_NODES-1, so each nodes will be linked to all other except itself
	std::size_t n_edges = 2*graph.getMpiCommunicator().getSize()-1;
	fpmas::graph::RingGraphBuilder<int> builder(n_edges, fpmas::graph::LOOP);
	DistributedNodeBuilder node_builder(
			2*graph.getMpiCommunicator().getSize(),
			graph.getMpiCommunicator()
			);

	builder.build(node_builder, 0, graph);

	for(auto node : graph.getLocationManager().getLocalNodes()) {
		ASSERT_THAT(node.second->getOutgoingEdges(0), SizeIs(n_edges));
		ASSERT_THAT(node.second->getIncomingEdges(0), SizeIs(n_edges));
	}
}

/*
 * Same as build_loop, but with a cycle.
 */
TEST(RingGraphBuilder, build_cycle) {
	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph(
			fpmas::communication::WORLD
			);

	fpmas::graph::RingGraphBuilder<int> builder(2);
	DistributedNodeBuilder node_builder(
			4*graph.getMpiCommunicator().getSize(),
			graph.getMpiCommunicator()
			);

	builder.build(node_builder, 0, graph);

	for(auto node : graph.getLocationManager().getLocalNodes()) {
		std::set<fpmas::graph::DistributedId> out_neighbors;
		for(auto edge : node.second->getOutgoingEdges(0))
			out_neighbors.insert(edge->getTargetNode()->getId());
		ASSERT_THAT(out_neighbors, SizeIs(4));

		std::set<fpmas::graph::DistributedId> in_neighbors;
		for(auto edge : node.second->getIncomingEdges(0))
			in_neighbors.insert(edge->getSourceNode()->getId());
		ASSERT_THAT(in_neighbors, SizeIs(4));
	}
}

/*
 * Same as build_complete_loop, but with a cycle.
 */
TEST(RingGraphBuilder, build_complete_cycle) {
	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph(
			fpmas::communication::WORLD
			);

	std::size_t n_edges = 2*graph.getMpiCommunicator().getSize()-1;
	fpmas::graph::RingGraphBuilder<int> builder(n_edges);
	DistributedNodeBuilder node_builder(
			2*graph.getMpiCommunicator().getSize(),
			graph.getMpiCommunicator()
			);

	builder.build(node_builder, 0, graph);

	for(auto node : graph.getLocationManager().getLocalNodes()) {
		ASSERT_THAT(node.second->getOutgoingEdges(0), SizeIs(2*n_edges));
		ASSERT_THAT(node.second->getIncomingEdges(0), SizeIs(2*n_edges));
	}
}
