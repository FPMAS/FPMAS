#include "fpmas/graph/graph_builder.h"
#include "fpmas/random/generator.h"
#include "fpmas/random/distribution.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/random/mock_random.h"

using namespace testing;

class MockBuildNode {
	public:
	std::vector<fpmas::api::graph::DistributedNode<int>*> nodes;

	fpmas::api::graph::DistributedNode<int>* build() {
		auto node = new MockDistributedNode<int>({0, 0});
		nodes.push_back(node);
		return node;
	}

	~MockBuildNode() {
		for(auto node : nodes)
			delete node;
	}
};

class MockLink {
	private:
		MockBuildNode& mock_build_node;
		std::size_t K;
		std::unordered_map<fpmas::api::graph::DistributedNode<int>*, std::set<fpmas::api::graph::DistributedNode<int>*>> edges;

	public:
		MockLink(MockBuildNode& mock_build_node, std::size_t K)
			: mock_build_node(mock_build_node), K(K) {}

		fpmas::api::graph::DistributedEdge<int>* link(
			fpmas::api::graph::DistributedNode<int>* src,
			fpmas::api::graph::DistributedNode<int>* tgt,
			Unused) {
			edges[src].insert(tgt);
			return nullptr;
		}

		void check() {
			std::size_t nodes_count = mock_build_node.nodes.size();
			// All nodes have been linked
			ASSERT_THAT(edges, SizeIs(nodes_count));
			for(auto edge_set : edges) {
				// Src is a node built by the GraphBuilder
				ASSERT_THAT(mock_build_node.nodes, Contains(edge_set.first));
				// The node is linked to 5 distinct nodes
				ASSERT_THAT(edge_set.second, SizeIs(std::min(nodes_count-1, K)));
				// The node is not linked to itself
				ASSERT_THAT(edge_set.second, Not(Contains(edge_set.first)));
				// Tgts are nodes built by the GraphBuilder
				for(auto tgt : edge_set.second)
					ASSERT_THAT(mock_build_node.nodes, Contains(tgt));
			}
		}
};

class FixedDegreeDistributionRandomGraphBuilder : public ::testing::Test {
	protected:
		fpmas::random::mt19937 generator;
		MockDistribution<std::size_t> dist;
		static const fpmas::api::graph::LayerId layer = 10;
		MockDistributedGraph<int> mock_graph;

		fpmas::graph::FixedDegreeDistributionRandomGraph<int>
			builder {generator, dist};

		MockBuildNode mock_build_node_action;

		std::vector<int> data;

		MockLink build_expectations(std::size_t nodes_count, std::size_t K) {
			// Each node will be linked to 5 others
			EXPECT_CALL(dist, call)
				.Times(AtLeast(nodes_count))
				.WillRepeatedly(Return(K));

			fpmas::random::UniformIntDistribution<int> random_data(0, 1000);

			MockLink mock_link(mock_build_node_action, K);

			EXPECT_CALL(mock_graph, link(_, _, layer))
				.Times(nodes_count * std::min(nodes_count-1, K))
				.WillRepeatedly(Invoke(&mock_link, &MockLink::link));
			for(std::size_t i = 0; i < nodes_count; i++) {
				data.push_back(random_data(generator));
			}
			EXPECT_CALL(mock_graph, buildNode_rv)
				.Times(nodes_count)
				.WillRepeatedly(Invoke(&mock_build_node_action, &MockBuildNode::build));

			return mock_link;
		}

};

TEST_F(FixedDegreeDistributionRandomGraphBuilder, regular_build) {
	static const std::size_t K = 5;
	static const std::size_t nodes_count = 20;

	auto mock_link = build_expectations(nodes_count, K);

	builder.build(data, layer, mock_graph);

	ASSERT_THAT(mock_build_node_action.nodes, SizeIs(nodes_count));

	mock_link.check();
}

TEST_F(FixedDegreeDistributionRandomGraphBuilder, build_without_enough_nodes) {
	static const std::size_t K = 10;
	static const std::size_t nodes_count = 8;

	auto mock_link = build_expectations(nodes_count, K);

	builder.build(data, layer, mock_graph);

	ASSERT_THAT(mock_build_node_action.nodes, SizeIs(nodes_count));

	mock_link.check();

}

TEST_F(FixedDegreeDistributionRandomGraphBuilder, edge_case) {
	static const std::size_t K = 10;

	data.push_back(1);
	EXPECT_CALL(dist, call)
		.WillRepeatedly(Return(K));
	EXPECT_CALL(mock_graph, buildNode_rv)
		.WillOnce(Invoke(&mock_build_node_action, &MockBuildNode::build));

	builder.build(data, layer, mock_graph);

	ASSERT_THAT(mock_build_node_action.nodes, SizeIs(1));
}
