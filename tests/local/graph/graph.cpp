#include "gtest/gtest.h"

#include "api/graph/basic_id.h"
#include "fpmas/graph/graph.h"
#include "fpmas/random/random.h"
#include "graph/mock_node.h"
#include "graph/mock_edge.h"
#include "utils/mock_callback.h"

using namespace testing;
using namespace fpmas::graph;

class GraphBaseTest : public Test {
	protected:
		Graph<MockNode<BasicId>, MockEdge<BasicId>> graph;
};

TEST_F(GraphBaseTest, insert_node) {
	BasicId id;
	MockCallback<MockNode<BasicId>*>* insert_callback
		= new MockCallback<MockNode<BasicId>*>;
	MockCallback<MockNode<BasicId>*>* erase_callback
		= new MockCallback<MockNode<BasicId>*>;
	graph.addCallOnInsertNode(insert_callback);
	graph.addCallOnEraseNode(erase_callback);
	for (int i = 0; i < 10; ++i) {
		auto node = new MockNode<BasicId>(++id);

		EXPECT_CALL(*node, getId).Times(AtLeast(2));
		EXPECT_CALL(*insert_callback, call(node));
		graph.insert(node);

		ASSERT_EQ(graph.getNodes().size(), i+1);
		ASSERT_EQ(graph.getNode(node->getId()), node);


		EXPECT_CALL(*node, getIncomingEdges()).Times(AnyNumber());
		EXPECT_CALL(*node, getIncomingEdges(_)).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingEdges()).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingEdges(_)).Times(AnyNumber());
		// Erase events should be triggered when the graph is deleted.
		EXPECT_CALL(*erase_callback, call(node))
			// Fake action, ensures node is not yet deleted when callback is
			// called
			.WillOnce([] (decltype(node) _node) {
					_node->getId();
					});
	}
}

TEST_F(GraphBaseTest, erase_node) {
	BasicId id;
	MockCallback<MockNode<BasicId>*>* erase_callback
		= new MockCallback<MockNode<BasicId>*>;
	graph.addCallOnEraseNode(erase_callback);
	std::array<MockNode<BasicId>*, 10> nodes;
	for (int i = 0; i < 10; ++i) {
		auto node = new NiceMock<MockNode<BasicId>>(++id);
		graph.insert(node);
		nodes[i] = node;
	}
	for(auto node : nodes) {
		BasicId node_id = node->getId();
		EXPECT_CALL(*erase_callback, call(node))
			// Fake action, ensures node is not yet deleted when callback is
			// called
			.WillOnce([] (decltype(node) _node) {
					_node->getId();
					});
		graph.erase(node);
		ASSERT_THAT(graph.getNodes(), Not(Contains(Pair(node_id, _))));
	}
}

TEST_F(GraphBaseTest, erase_edge) {
	BasicId id;
	MockNode<BasicId>* src = new NiceMock<MockNode<BasicId>>(++id);
	MockNode<BasicId>* tgt = new NiceMock<MockNode<BasicId>>(++id);
	MockEdge<BasicId>* edge = new NiceMock<MockEdge<BasicId>>(++id, 2);

	graph.insert(src);
	graph.insert(tgt);
	edge->src = src;
	edge->tgt = tgt;
	graph.insert(edge);

	EXPECT_CALL(*src, unlinkOut(edge));
	EXPECT_CALL(*src, unlinkIn).Times(0);
	EXPECT_CALL(*tgt, unlinkOut).Times(0);
	EXPECT_CALL(*tgt, unlinkIn(edge));

	graph.erase(edge);
}

class GraphBaseMoveTest : public GraphBaseTest {
	protected:
		BasicId node_id;
		BasicId edge_id;
		fpmas::random::mt19937_64 rd;

		void build_graph(
				fpmas::api::graph::Graph<MockNode<BasicId>, MockEdge<BasicId>>& graph,
				std::size_t num_callbacks, std::size_t num_nodes, std::size_t num_edges) {
			for(std::size_t i = 0; i < num_callbacks; i++) {
				graph.addCallOnInsertNode(new NiceMock<MockCallback<MockNode<BasicId>*>>);
				graph.addCallOnEraseNode(new NiceMock<MockCallback<MockNode<BasicId>*>>);
				graph.addCallOnInsertEdge(new NiceMock<MockCallback<MockEdge<BasicId>*>>);
				graph.addCallOnEraseEdge(new NiceMock<MockCallback<MockEdge<BasicId>*>>);
			}
			std::vector<MockNode<BasicId>*> local_nodes;
			for(std::size_t i = 0; i < num_nodes; i++) {
				auto node = new NiceMock<MockNode<BasicId>>(++node_id);
				local_nodes.push_back(node);
				graph.insert(node);
			}
			for(std::size_t i = 0; i < num_edges; i++) {
				auto edge = new NiceMock<MockEdge<BasicId>>(++edge_id, 0);
				fpmas::random::UniformIntDistribution<std::size_t> rd_id(0, local_nodes.size()-1);
				edge->src = local_nodes[rd_id(rd)];
				edge->tgt = local_nodes[rd_id(rd)];

				graph.insert(edge);
			}
		}
	void SetUp() override {
		build_graph(this->graph, 2, 10, 20);
	}
};

TEST_F(GraphBaseMoveTest, move_constructor) {
	auto insert_node = this->graph.onInsertNodeCallbacks();
	auto insert_edge = this->graph.onInsertEdgeCallbacks();
	auto erase_node = this->graph.onEraseNodeCallbacks();
	auto erase_edge = this->graph.onEraseEdgeCallbacks();
	auto nodes = this->graph.getNodes();
	auto edges = this->graph.getEdges();

	Graph<MockNode<BasicId>, MockEdge<BasicId>> new_graph(std::move(this->graph));

	ASSERT_THAT(this->graph.getNodes(), IsEmpty());
	ASSERT_THAT(new_graph.getNodes(), SizeIs(10));
	ASSERT_THAT(new_graph.getNodes(), UnorderedElementsAreArray(nodes));
	ASSERT_THAT(this->graph.getEdges(), IsEmpty());
	ASSERT_THAT(new_graph.getEdges(), SizeIs(20));
	ASSERT_THAT(new_graph.getEdges(), UnorderedElementsAreArray(edges));

	ASSERT_THAT(this->graph.onInsertNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onInsertEdgeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseEdgeCallbacks(), IsEmpty());

	ASSERT_THAT(new_graph.onInsertNodeCallbacks(), UnorderedElementsAreArray(insert_node));
	ASSERT_THAT(new_graph.onEraseNodeCallbacks(), UnorderedElementsAreArray(erase_node));
	ASSERT_THAT(new_graph.onInsertEdgeCallbacks(), UnorderedElementsAreArray(insert_edge));
	ASSERT_THAT(new_graph.onEraseEdgeCallbacks(), UnorderedElementsAreArray(erase_edge));
}

TEST_F(GraphBaseMoveTest, move_assign) {
	auto insert_node = this->graph.onInsertNodeCallbacks();
	auto insert_edge = this->graph.onInsertEdgeCallbacks();
	auto erase_node = this->graph.onEraseNodeCallbacks();
	auto erase_edge = this->graph.onEraseEdgeCallbacks();
	auto nodes = this->graph.getNodes();
	auto edges = this->graph.getEdges();

	Graph<MockNode<BasicId>, MockEdge<BasicId>> new_graph;

	// Initializes the graph
	build_graph(new_graph, 2, 5, 14);
	// Moves graph into new_graph.
	// new_graph must be cleared.
	new_graph = std::move(this->graph);

	ASSERT_THAT(this->graph.getNodes(), IsEmpty());
	ASSERT_THAT(new_graph.getNodes(), SizeIs(10));
	ASSERT_THAT(new_graph.getNodes(), UnorderedElementsAreArray(nodes));
	ASSERT_THAT(this->graph.getEdges(), IsEmpty());
	ASSERT_THAT(new_graph.getEdges(), SizeIs(20));
	ASSERT_THAT(new_graph.getEdges(), UnorderedElementsAreArray(edges));
	ASSERT_THAT(this->graph.onInsertNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseNodeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onInsertEdgeCallbacks(), IsEmpty());
	ASSERT_THAT(this->graph.onEraseEdgeCallbacks(), IsEmpty());

	ASSERT_THAT(new_graph.onInsertNodeCallbacks(), UnorderedElementsAreArray(insert_node));
	ASSERT_THAT(new_graph.onEraseNodeCallbacks(), UnorderedElementsAreArray(erase_node));
	ASSERT_THAT(new_graph.onInsertEdgeCallbacks(), UnorderedElementsAreArray(insert_edge));
	ASSERT_THAT(new_graph.onEraseEdgeCallbacks(), UnorderedElementsAreArray(erase_edge));
}
