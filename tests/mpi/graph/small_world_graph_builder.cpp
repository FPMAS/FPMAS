#include "fpmas/graph/small_world_graph_builder.h"
#include "fpmas/api/graph/location_state.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/io/json_output.h"
#include "fpmas/synchro/hard/hard_sync_mode.h"
#include "fpmas/graph/analysis.h"
#include "fpmas/graph/graph_builder.h"

#include "gmock/gmock.h"
#include <algorithm>
#include <gmock/gmock-matchers.h>
#include <unordered_map>

using namespace testing;

TEST(SmallWorldGraphBuilder, p_0) {
	fpmas::graph::SmallWorldGraphBuilder<int> graph_builder(0, 4);

	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph(
			fpmas::communication::WORLD
			);

	// Builder with 4 nodes by process
	fpmas::graph::DefaultDistributedNodeBuilder<int> node_builder(
			4*graph.getMpiCommunicator().getSize(),
			graph.getMpiCommunicator()
			);

	graph_builder.build(node_builder, 0, graph);

	// The ring is preserved
	ASSERT_EQ(fpmas::graph::clustering_coefficient(graph, 0), 0.5);
	ASSERT_EQ(fpmas::graph::node_count(graph), 4*graph.getMpiCommunicator().getSize());
	ASSERT_EQ(fpmas::graph::edge_count(graph), 4*graph.getMpiCommunicator().getSize()*4);
}

TEST(SmallWorldGraphBuilder, p_01) {
	std::size_t n = 4*fpmas::communication::WORLD.getSize();
	std::size_t k = fpmas::communication::WORLD.getSize();
	fpmas::graph::SmallWorldGraphBuilder<int> graph_builder(0.1, k);

	fpmas::graph::DistributedGraph<int, fpmas::synchro::HardSyncMode> graph(
			fpmas::communication::WORLD
			);

	// Builder with 4 nodes by process
	fpmas::graph::DefaultDistributedNodeBuilder<int> node_builder(
			n, graph.getMpiCommunicator());

	graph_builder.build(node_builder, 0, graph);

	// Ensures that there is no duplicate edges
	for(auto node : graph.getLocationManager().getLocalNodes()) {
		std::size_t outcount = node.second->getOutgoingEdges(0).size();
		std::set<DistributedId> distinct_out_neighbors;
		for(auto edge : node.second->getOutgoingEdges(0))
			distinct_out_neighbors.insert(edge->getTargetNode()->getId());
		ASSERT_THAT(distinct_out_neighbors, SizeIs(outcount));

		std::size_t incount = node.second->getIncomingEdges(0).size();
		std::set<DistributedId> distinct_in_neighbors;
		for(auto edge : node.second->getIncomingEdges(0))
			distinct_in_neighbors.insert(edge->getSourceNode()->getId());
		ASSERT_THAT(distinct_in_neighbors, SizeIs(incount));
	}

	// The ring is preserved
	ASSERT_EQ(fpmas::graph::node_count(graph), n);
	ASSERT_EQ(fpmas::graph::edge_count(graph), n*k);

	/*
	 * Output that might be useful to produce doc:
	 */
	/*
	 *fpmas::io::FileOutput file("graph." + std::to_string(fpmas::communication::WORLD.getRank()) + ".json");
	 *fpmas::io::JsonOutput<const fpmas::api::graph::DistributedGraph<int>&> json_output(
	 *        file, [&graph] () -> const fpmas::api::graph::DistributedGraph<int>& {return graph;}
	 *        );
	 *json_output.dump();
	 */
}
