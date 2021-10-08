#include "fpmas/graph/graph_builder.h"
#include "fpmas/graph/distributed_graph.h"
#include "fpmas/random/random.h"
#include "fpmas/synchro/ghost/ghost_mode.h"
#include "random/mock_random.h"

#include "gmock/gmock.h"

using namespace testing;

class DistributedNodeBuilder : public fpmas::graph::DistributedNodeBuilder<int> {
	using fpmas::graph::DistributedNodeBuilder<int>::DistributedNodeBuilder;

	fpmas::api::graph::DistributedNode<int>*
		buildNode(fpmas::api::graph::DistributedGraph<int>& graph) override {
			local_node_count--;
			return graph.buildNode({});
		}

	fpmas::api::graph::DistributedNode<int>*
		buildDistantNode(
				fpmas::api::graph::DistributedId id, int location,
				fpmas::api::graph::DistributedGraph<int>& graph) override {
			auto node = new fpmas::graph::DistributedNode<int>(id, 0);
			node->setLocation(location);
			return graph.insertDistant(node);
		}
};

TEST(DistributedUniformGraphBuilder, build) {
	static std::size_t NODE_COUNT = 200;
	static std::size_t NUM_EDGE_BY_NODE = 6;
	fpmas::graph::DistributedGraph<int, fpmas::synchro::GhostMode> graph(fpmas::communication::WORLD);

	fpmas::random::DistributedGenerator<> generator;
	NiceMock<MockDistribution<std::size_t>> edge_dist;
	
	ON_CALL(edge_dist, call)
		.WillByDefault(Return(NUM_EDGE_BY_NODE));

	DistributedNodeBuilder node_builder(NODE_COUNT, graph.getMpiCommunicator());
	fpmas::graph::DistributedUniformGraphBuilder<int> builder(generator, edge_dist);

	builder.build(node_builder, 2, graph);

	fpmas::communication::TypedMpi<std::size_t> mpi(graph.getMpiCommunicator());

	std::size_t node_count = fpmas::communication::all_reduce(
			mpi,
			graph.getLocationManager().getLocalNodes().size()
			);

	ASSERT_GE(graph.getLocationManager().getLocalNodes().size(), 1);
	ASSERT_EQ(node_count, NODE_COUNT);

	std::set<int> nodes_on_other_procs;
	for(auto node : graph.getLocationManager().getLocalNodes()) {
		auto edges = node.second->getOutgoingEdges(2);
		ASSERT_THAT(edges.size(), NUM_EDGE_BY_NODE);
		for(auto edge : node.second->getOutgoingEdges()) {
			nodes_on_other_procs.insert(edge->getTargetNode()->location());
		}
		ASSERT_THAT(std::set<fpmas::api::graph::DistributedEdge<int>*>(
					edges.begin(),
					edges.end()
					).size(), NUM_EDGE_BY_NODE);
	}

	ASSERT_THAT(
			nodes_on_other_procs,
			Not(UnorderedElementsAre(graph.getMpiCommunicator().getRank()))
			);
}

TEST(DistributedClusteredGraphBuilder, build) {
	static std::size_t NODE_COUNT = 1000;
	static std::size_t NUM_EDGE_BY_NODE = 6;
	fpmas::graph::DistributedGraph<int, fpmas::synchro::GhostMode> graph(
			fpmas::communication::WORLD
			);

	fpmas::random::DistributedGenerator<> generator;
	NiceMock<MockDistribution<std::size_t>> edge_dist;
	
	ON_CALL(edge_dist, call)
		.WillByDefault(Return(NUM_EDGE_BY_NODE));

	fpmas::random::UniformRealDistribution<double> xy_dist(0, 10);

	DistributedNodeBuilder node_builder(NODE_COUNT, graph.getMpiCommunicator());

	fpmas::graph::DistributedClusteredGraphBuilder<int> builder(
			generator, edge_dist, xy_dist, xy_dist
			);

	builder.build(node_builder, 2, graph);

	fpmas::communication::TypedMpi<std::size_t> mpi(graph.getMpiCommunicator());

	std::size_t node_count = fpmas::communication::all_reduce(
			mpi,
			graph.getLocationManager().getLocalNodes().size()
			);

	ASSERT_GE(graph.getLocationManager().getLocalNodes().size(), 1);
	ASSERT_EQ(node_count, NODE_COUNT);

	std::set<int> nodes_on_other_procs;
	for(auto node : graph.getLocationManager().getLocalNodes()) {
		auto edges = node.second->getOutgoingEdges(2);
		ASSERT_THAT(edges.size(), NUM_EDGE_BY_NODE);
		for(auto edge : node.second->getOutgoingEdges()) {
			nodes_on_other_procs.insert(edge->getTargetNode()->location());
		}
		ASSERT_THAT(std::set<fpmas::api::graph::DistributedEdge<int>*>(
					edges.begin(),
					edges.end()
					).size(), NUM_EDGE_BY_NODE);
	}

	ASSERT_THAT(
			nodes_on_other_procs,
			Not(UnorderedElementsAre(graph.getMpiCommunicator().getRank()))
			);
}
