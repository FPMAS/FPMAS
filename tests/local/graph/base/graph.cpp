#include "gtest/gtest.h"

#include "graph/base/graph.h"
#include "api/graph/base/basic_id.h"
#include "../mocks/graph/base/mock_node.h"
#include "../mocks/graph/base/mock_arc.h"
#include "../mocks/utils/mock_callback.h"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::Contains;
using ::testing::Expectation;
using ::testing::Not;
using ::testing::Pair;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

template<typename NodeType, typename ArcType>
class MockGraph : 
	public virtual fpmas::graph::base::Graph<NodeType, ArcType> {
		typedef fpmas::graph::base::Graph<NodeType, ArcType>
		GraphBase;
		using typename GraphBase::NodeIdType;
		using typename GraphBase::NodeMap;

		using typename GraphBase::ArcIdType;
		using typename GraphBase::ArcMap;

		public:
			MOCK_METHOD(void, removeNode, (NodeType*), (override));
			MOCK_METHOD(void, unlink, (ArcType*), (override));

};

class GraphBaseTest : public ::testing::Test {
	protected:
		MockGraph<MockNode<BasicId>, MockArc<BasicId>> graph;
		MockCallback<MockNode<BasicId>*>* insert_callback = new MockCallback<MockNode<BasicId>*>;
		MockCallback<MockNode<BasicId>*>* erase_callback = new MockCallback<MockNode<BasicId>*>;
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


		EXPECT_CALL(*node, getIncomingArcs()).Times(AnyNumber());
		EXPECT_CALL(*node, getIncomingArcs(_)).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingArcs()).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingArcs(_)).Times(AnyNumber());
		// Erase events should be triggered when the graph is deleted.
		Expectation callback = EXPECT_CALL(*erase_callback, call(node));
		EXPECT_CALL(*node, die).After(callback);
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

		EXPECT_CALL(*node, getIncomingArcs()).Times(AnyNumber());
		EXPECT_CALL(*node, getIncomingArcs(_)).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingArcs()).Times(AnyNumber());
		EXPECT_CALL(*node, getOutgoingArcs(_)).Times(AnyNumber());
	}
	for(auto node : nodes) {
		BasicId node_id = node->getId();
		Expectation callback = EXPECT_CALL(*erase_callback, call(node));
		EXPECT_CALL(*node, die).After(callback);
		graph.erase(node);
		ASSERT_THAT(graph.getNodes(), Not(Contains(Pair(node_id, _))));
	}
	delete insert_callback;
}

class GraphBaseEraseArcTest : public ::testing::Test {
	protected:
		MockGraph<MockNode<BasicId>, MockArc<BasicId>> graph;
		BasicId id;
		MockNode<BasicId>* src = new MockNode<BasicId>(++id);
		MockNode<BasicId>* tgt = new MockNode<BasicId>(++id);
		MockArc<BasicId>* arc = new MockArc<BasicId>(++id, 2);

		void SetUp() override {
			graph.insert(src);
			graph.insert(tgt);
			arc->src = src;
			arc->tgt = tgt;
			graph.insert(arc);

			EXPECT_CALL(*src, getIncomingArcs()).Times(AnyNumber());
			EXPECT_CALL(*src, getIncomingArcs(_)).Times(AnyNumber());
			EXPECT_CALL(*src, getOutgoingArcs()).Times(AnyNumber());
			EXPECT_CALL(*src, getOutgoingArcs(_)).Times(AnyNumber());
			EXPECT_CALL(*tgt, getIncomingArcs()).Times(AnyNumber());
			EXPECT_CALL(*tgt, getIncomingArcs(_)).Times(AnyNumber());
			EXPECT_CALL(*tgt, getOutgoingArcs()).Times(AnyNumber());
			EXPECT_CALL(*tgt, getOutgoingArcs(_)).Times(AnyNumber());
		}
};

TEST_F(GraphBaseEraseArcTest, erase) {
	EXPECT_CALL(*src, unlinkOut(arc));
	EXPECT_CALL(*src, unlinkIn).Times(0);
	EXPECT_CALL(*tgt, unlinkOut).Times(0);
	EXPECT_CALL(*tgt, unlinkIn(arc));

	graph.erase(arc);
}
