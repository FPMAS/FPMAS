#include "fpmas/graph/graph_builder.h"
#include "fpmas/random/generator.h"
#include "fpmas/random/distribution.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/random/mock_random.h"

using namespace testing;

class FakeNodeBuilder : public fpmas::api::graph::NodeBuilder<int> {
	public:
		std::vector<fpmas::api::graph::DistributedNode<int>*> nodes;
		std::size_t count;

		FakeNodeBuilder(std::size_t count)
			: count(count) {}

		fpmas::api::graph::DistributedNode<int>* buildNode(fpmas::api::graph::DistributedGraph<int>&) override {
			auto node = new MockDistributedNode<int>({0, 0});
			nodes.push_back(node);
			count--;
			return node;
		}

		std::size_t nodeCount() override {
			return count;
		}

		~FakeNodeBuilder() {
			for(auto node : nodes)
				delete node;
		}
};

class MockLink {
	private:
		FakeNodeBuilder& fake_node_builder;

	public:
		std::unordered_map<fpmas::api::graph::DistributedNode<int>*, std::set<fpmas::api::graph::DistributedNode<int>*>> edges;

		MockLink(FakeNodeBuilder& fake_node_builder)
			: fake_node_builder(fake_node_builder) {}

		fpmas::api::graph::DistributedEdge<int>* link(
				fpmas::api::graph::DistributedNode<int>* src,
				fpmas::api::graph::DistributedNode<int>* tgt,
				Unused) {
			edges[src].insert(tgt);
			return nullptr;
		}

		void check(std::vector<fpmas::api::graph::DistributedNode<int>*> available_target_nodes, std::size_t edge_count) {
			// All nodes have been linked
			ASSERT_THAT(edges, SizeIs(fake_node_builder.nodes.size()));
			for(auto edge_set : edges) {
				// Src is a node built by the GraphBuilder
				ASSERT_THAT(fake_node_builder.nodes, Contains(edge_set.first));
				// The node is linked to K distinct nodes
				ASSERT_THAT(edge_set.second, SizeIs(edge_count));
				// The node is not linked to itself
				ASSERT_THAT(edge_set.second, Not(Contains(edge_set.first)));
				// Tgts are nodes built by the GraphBuilder
				for(auto tgt : edge_set.second)
					ASSERT_THAT(available_target_nodes, Contains(tgt));
			}
		}
};

class UniformGraphBuilder : public ::testing::Test {
	protected:
		fpmas::random::mt19937 generator;
		MockDistribution<std::size_t> dist;
		static const fpmas::api::graph::LayerId layer = 10;
		MockDistributedGraph<int> mock_graph;

		fpmas::graph::UniformGraphBuilder<int>
			builder {generator, dist};

		MockLink build_expectations(FakeNodeBuilder& node_builder, std::size_t K) {
			// Each node will be linked to 5 others
			EXPECT_CALL(dist, call)
				.Times(AtLeast(node_builder.nodeCount()))
				.WillRepeatedly(Return(K));

			MockLink mock_link(node_builder);

			EXPECT_CALL(mock_graph, link(_, _, layer))
				.Times(node_builder.nodeCount() * std::min(node_builder.nodeCount()-1, K))
				.WillRepeatedly(Invoke(&mock_link, &MockLink::link));

			return mock_link;
		}

};

TEST_F(UniformGraphBuilder, regular_build) {
	static const std::size_t K = 5;
	static const std::size_t nodes_count = 20;

	FakeNodeBuilder node_builder(nodes_count);
	auto mock_link = build_expectations(node_builder, K);

	auto built_nodes = builder.build(node_builder, layer, mock_graph);
	ASSERT_THAT(built_nodes, UnorderedElementsAreArray(node_builder.nodes));

	ASSERT_THAT(node_builder.nodes, SizeIs(nodes_count));

	mock_link.check(node_builder.nodes, K);
}

TEST_F(UniformGraphBuilder, build_without_enough_nodes) {
	static const std::size_t K = 10;
	static const std::size_t nodes_count = 8;

	FakeNodeBuilder node_builder(nodes_count);
	auto mock_link = build_expectations(node_builder, K);

	auto built_nodes = builder.build(node_builder, layer, mock_graph);
	ASSERT_THAT(built_nodes, UnorderedElementsAreArray(node_builder.nodes));

	ASSERT_THAT(node_builder.nodes, SizeIs(nodes_count));

	mock_link.check(node_builder.nodes, 7);

}

TEST_F(UniformGraphBuilder, edge_case) {
	static const std::size_t K = 10;

	FakeNodeBuilder node_builder(1);
	EXPECT_CALL(dist, call)
		.WillRepeatedly(Return(K));

	auto built_nodes = builder.build(node_builder, layer, mock_graph);
	ASSERT_THAT(built_nodes, UnorderedElementsAreArray(node_builder.nodes));

	ASSERT_THAT(node_builder.nodes, SizeIs(1));
}

class ClusteredGraphBuilder : public ::testing::Test {
	protected:
		fpmas::random::mt19937 generator;
		MockDistribution<std::size_t> edge_dist;
		MockDistribution<double> x_dist;
		MockDistribution<double> y_dist;
		static const fpmas::api::graph::LayerId layer = 10;
		MockDistributedGraph<int> mock_graph;

		fpmas::graph::ClusteredGraphBuilder<int> graph_builder
		{generator, edge_dist, x_dist, y_dist};


};

TEST_F(ClusteredGraphBuilder, regular_build) {
	EXPECT_CALL(x_dist, call)
		.WillOnce(Return(0))
		.WillOnce(Return(-2))
		.WillOnce(Return(2))
		.WillOnce(Return(3));

	EXPECT_CALL(y_dist, call)
		.WillOnce(Return(0))
		.WillOnce(Return(1))
		.WillOnce(Return(1))
		.WillOnce(Return(-3));

	std::size_t K = 2;
	EXPECT_CALL(edge_dist, call)
		.WillRepeatedly(Return(K));

	FakeNodeBuilder node_builder(4);

	MockLink mock_link (node_builder);

	EXPECT_CALL(mock_graph, link(_, _, layer))
		.Times(8)
		.WillRepeatedly(Invoke(&mock_link, &MockLink::link));

	auto built_nodes = graph_builder.build(node_builder, layer, mock_graph);
	ASSERT_THAT(built_nodes, UnorderedElementsAreArray(node_builder.nodes));

	auto node_0 = node_builder.nodes[0];
	auto node_1 = node_builder.nodes[1];
	auto node_2 = node_builder.nodes[2];
	auto node_3 = node_builder.nodes[3];

	mock_link.check(node_builder.nodes, K);

	ASSERT_THAT(mock_link.edges[node_0], UnorderedElementsAre(node_1, node_2));
	ASSERT_THAT(mock_link.edges[node_1], UnorderedElementsAre(node_0, node_2));
	ASSERT_THAT(mock_link.edges[node_2], UnorderedElementsAre(node_0, node_1));
	ASSERT_THAT(mock_link.edges[node_3], UnorderedElementsAre(node_0, node_2));
}

TEST_F(ClusteredGraphBuilder, build_without_enough_nodes) {
	EXPECT_CALL(x_dist, call)
		.WillOnce(Return(0))
		.WillOnce(Return(-2))
		.WillOnce(Return(2))
		.WillOnce(Return(3));

	EXPECT_CALL(y_dist, call)
		.WillOnce(Return(0))
		.WillOnce(Return(1))
		.WillOnce(Return(1))
		.WillOnce(Return(-3));

	std::size_t K = 8; // but only 3 available
	EXPECT_CALL(edge_dist, call)
		.WillRepeatedly(Return(K));

	FakeNodeBuilder node_builder(4);

	MockLink mock_link (node_builder);

	EXPECT_CALL(mock_graph, link(_, _, layer))
		.Times(12)
		.WillRepeatedly(Invoke(&mock_link, &MockLink::link));

	auto built_nodes = graph_builder.build(node_builder, layer, mock_graph);
	ASSERT_THAT(built_nodes, UnorderedElementsAreArray(node_builder.nodes));

	mock_link.check(node_builder.nodes, 3);
}

class BipartiteGraphBuilder : public ::testing::Test {
	protected:
		fpmas::random::mt19937 generator;
		MockDistribution<std::size_t> edge_dist;
		static const fpmas::api::graph::LayerId layer = 10;
		MockDistributedGraph<int> mock_graph;

		std::vector<fpmas::api::graph::DistributedNode<int>*> base_nodes {{
			new MockDistributedNode<int>, new MockDistributedNode<int>,
				new MockDistributedNode<int>, new MockDistributedNode<int>
		}};
		fpmas::graph::BipartiteGraphBuilder<int> graph_builder
		{generator, edge_dist, base_nodes};

		void TearDown() override {
			for(auto node : base_nodes)
				delete node;
		}
};

TEST_F(BipartiteGraphBuilder, regular_build) {
	std::size_t K = 2;
	// Dist => 2 out edges by node
	EXPECT_CALL(edge_dist, call)
		.WillRepeatedly(Return(K));

	// Build three nodes, that must be connected to the 4 base nodes
	FakeNodeBuilder node_builder(3);

	// 2 out edges by node
	MockLink mock_link (node_builder);

	// 3*2 edges created
	EXPECT_CALL(mock_graph, link(_, _, layer))
		.Times(6)
		.WillRepeatedly(Invoke(&mock_link, &MockLink::link));

	auto built_nodes = graph_builder.build(node_builder, layer, mock_graph);
	ASSERT_THAT(built_nodes, UnorderedElementsAreArray(node_builder.nodes));

	mock_link.check(base_nodes, K);
}

TEST_F(BipartiteGraphBuilder, build_without_enough_nodes) {
	std::size_t K = 6;
	// Dist => 6 out edges by node
	EXPECT_CALL(edge_dist, call)
		.WillRepeatedly(Return(K));

	// Build three nodes, that must be connected to the 4 base nodes
	FakeNodeBuilder node_builder(3);

	// Only 4 nodes available, not 6
	MockLink mock_link (node_builder);

	// 3*4 edges created
	EXPECT_CALL(mock_graph, link(_, _, layer))
		.Times(12)
		.WillRepeatedly(Invoke(&mock_link, &MockLink::link));

	auto built_nodes = graph_builder.build(node_builder, layer, mock_graph);
	ASSERT_THAT(built_nodes, UnorderedElementsAreArray(node_builder.nodes));

	mock_link.check(base_nodes, 4);

	// All nodes are necessarily linked to the 4 available nodes
	for(auto node : mock_link.edges) {
		ASSERT_THAT(node.second, UnorderedElementsAreArray(base_nodes));
	}
}
