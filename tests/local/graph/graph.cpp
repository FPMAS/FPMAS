#include "gtest/gtest.h"

#include "api/graph/basic_id.h"
#include "fpmas/graph/graph.h"
#include "../mocks/graph/mock_node.h"
#include "../mocks/graph/mock_edge.h"
#include "../mocks/utils/mock_callback.h"

using namespace testing;

/*
 *
 *template<typename NodeType, typename EdgeType>
 *class MockGraph : 
 *    public virtual fpmas::graph::Graph<NodeType, EdgeType> {
 *        typedef fpmas::graph::Graph<NodeType, EdgeType>
 *        GraphBase;
 *        using typename GraphBase::NodeIdType;
 *        using typename GraphBase::NodeMap;
 *
 *        using typename GraphBase::EdgeIdType;
 *        using typename GraphBase::EdgeMap;
 *
 *        public:
 *            MOCK_METHOD(void, removeNode, (NodeType*), (override));
 *            MOCK_METHOD(void, unlink, (EdgeType*), (override));
 *
 *};
 */

class GraphBaseTest : public Test {
	protected:
		fpmas::graph::Graph<MockNode<BasicId>, MockEdge<BasicId>> graph;
		MockCallback<MockNode<BasicId>*>* insert_callback
			= new MockCallback<MockNode<BasicId>*>;
		MockCallback<MockNode<BasicId>*>* erase_callback
			= new MockCallback<MockNode<BasicId>*>;
};

TEST_F(GraphBaseTest, insert_node) {
	BasicId id;
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
	graph.addCallOnEraseNode(erase_callback);
	std::array<MockNode<BasicId>*, 10> nodes;
	for (int i = 0; i < 10; ++i) {
		auto node = new MockNode<BasicId>(++id);
		graph.insert(node);
		nodes[i] = node;

		EXPECT_CALL(*node, getIncomingEdges()).Times(AnyNumber());
		EXPECT_CALL(*node, getIncomingEdges(_)).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingEdges()).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingEdges(_)).Times(AnyNumber());
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
	delete insert_callback;
}

class GraphBaseEraseEdgeTest : public Test {
	protected:
		fpmas::graph::Graph<MockNode<BasicId>, MockEdge<BasicId>> graph;
		BasicId id;
		MockNode<BasicId>* src = new MockNode<BasicId>(++id);
		MockNode<BasicId>* tgt = new MockNode<BasicId>(++id);
		MockEdge<BasicId>* edge = new MockEdge<BasicId>(++id, 2);

		void SetUp() override {
			graph.insert(src);
			graph.insert(tgt);
			edge->src = src;
			edge->tgt = tgt;
			graph.insert(edge);

			EXPECT_CALL(*src, getIncomingEdges()).Times(AnyNumber());
			EXPECT_CALL(*src, getIncomingEdges(_)).Times(AnyNumber());
			EXPECT_CALL(*src, getOutgoingEdges()).Times(AnyNumber());
			EXPECT_CALL(*src, getOutgoingEdges(_)).Times(AnyNumber());
			EXPECT_CALL(*tgt, getIncomingEdges()).Times(AnyNumber());
			EXPECT_CALL(*tgt, getIncomingEdges(_)).Times(AnyNumber());
			EXPECT_CALL(*tgt, getOutgoingEdges()).Times(AnyNumber());
			EXPECT_CALL(*tgt, getOutgoingEdges(_)).Times(AnyNumber());
		}
};

TEST_F(GraphBaseEraseEdgeTest, erase) {
	EXPECT_CALL(*src, unlinkOut(edge));
	EXPECT_CALL(*src, unlinkIn).Times(0);
	EXPECT_CALL(*tgt, unlinkOut).Times(0);
	EXPECT_CALL(*tgt, unlinkIn(edge));

	graph.erase(edge);
}
