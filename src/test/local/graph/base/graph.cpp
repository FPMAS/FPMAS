#include "gtest/gtest.h"

#include "graph/base/graph.h"
#include "graph/base/basic_id.h"
#include "../mocks/graph/base/mock_node.h"
#include "../mocks/graph/base/mock_arc.h"
#include "../mocks/utils/mock_callback.h"

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::AnyNumber;
using ::testing::AtLeast;

using FPMAS::graph::base::BasicId;

template<typename NodeType, typename ArcType>
class MockGraph : 
	public virtual FPMAS::graph::base::Graph<NodeType, ArcType> {
		typedef FPMAS::graph::base::Graph<NodeType, ArcType>
		GraphBase;
		using typename GraphBase::NodeIdType;
		using typename GraphBase::NodeMap;

		using typename GraphBase::ArcIdType;
		using typename GraphBase::ArcMap;

		public:
			MOCK_METHOD(void, removeNode, (NodeType*), (override));
			MOCK_METHOD(void, unlink, (ArcType*), (override));

};
TEST(GraphBaseTest, buildDefaultNode) {
	MockGraph<MockNode<BasicId>, MockArc<BasicId>> graph;

	BasicId id;
	for (int i = 0; i < 10; ++i) {
		//MockNodeCallback* insert = new MockNodeCallback;
		auto node = new MockNode<BasicId>(++id);

		EXPECT_CALL(*node, getId).Times(AtLeast(2));
		//EXPECT_CALL(*insert, call);
		//graph.addOnInsert(insert);
		graph.insert(node);

		ASSERT_EQ(graph.getNodes().size(), i+1);
		ASSERT_EQ(graph.getNode(node->getId()), node);
		//delete insert;
	}
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
		}
};

TEST_F(GraphBaseEraseArcTest, erase) {
	EXPECT_CALL(*src, unlinkOut(arc));
	EXPECT_CALL(*src, unlinkIn).Times(0);
	EXPECT_CALL(*tgt, unlinkOut).Times(0);
	EXPECT_CALL(*tgt, unlinkIn(arc));

	graph.erase(arc);
}
