#include "fpmas/graph/ring_graph_builder.h"
#include "fpmas/graph/analysis.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/graph/graph_builder.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

using namespace testing;

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
	fpmas::graph::DefaultDistributedNodeBuilder<int> node_builder(
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
	// clustering coefficient check
	ASSERT_FLOAT_EQ(fpmas::graph::clustering_coefficient(graph, 0), 0.25);
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
	fpmas::graph::DefaultDistributedNodeBuilder<int> node_builder(
			2*graph.getMpiCommunicator().getSize(),
			graph.getMpiCommunicator()
			);

	builder.build(node_builder, 0, graph);

	for(auto node : graph.getLocationManager().getLocalNodes()) {
		ASSERT_THAT(node.second->getOutgoingEdges(0), SizeIs(n_edges));
		ASSERT_THAT(node.second->getIncomingEdges(0), SizeIs(n_edges));
	}

	// clustering coefficient check
	ASSERT_FLOAT_EQ(fpmas::graph::clustering_coefficient(graph, 0), 1);
}

/*
 * Same as build_loop, but with a cycle.
 */
TEST(RingGraphBuilder, build_cycle) {
	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph(
			fpmas::communication::WORLD
			);

	fpmas::graph::RingGraphBuilder<int> builder(2);
	fpmas::graph::DefaultDistributedNodeBuilder<int> node_builder(
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

	// clustering coefficient check
	ASSERT_FLOAT_EQ(fpmas::graph::clustering_coefficient(graph, 0), 0.5);
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
	fpmas::graph::DefaultDistributedNodeBuilder<int> node_builder(
			2*graph.getMpiCommunicator().getSize(),
			graph.getMpiCommunicator()
			);

	builder.build(node_builder, 0, graph);

	for(auto node : graph.getLocationManager().getLocalNodes()) {
		ASSERT_THAT(node.second->getOutgoingEdges(0), SizeIs(2*n_edges));
		ASSERT_THAT(node.second->getIncomingEdges(0), SizeIs(2*n_edges));
	}

	// clustering coefficient check
	// Extreme case where c=2 since every node is connected to all others with an
	// in/out edge so all edges are actually duplicated.
	ASSERT_FLOAT_EQ(fpmas::graph::clustering_coefficient(graph, 0), 2);
}
