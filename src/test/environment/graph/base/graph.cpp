#include "gtest/gtest.h"
#include "environment/graph/base/graph.h"

#include "sample_graph.h"

using FPMAS::graph::Graph;
using FPMAS::graph::Node;
using FPMAS::graph::Arc;

/*
 * Trivial constructor test.
 */
TEST(GraphTest, buildTest) {
	Graph<int> graph = Graph<int>();
}

TEST_F(SampleGraphTest, buildSimpleGraph) {
	testSampleGraphStructure(&sampleGraph);
}

TEST_F(SampleGraphTest, deleteNodeTest) {
	sampleGraph.removeNode(1ul);

	ASSERT_EQ(sampleGraph.getNodes().size(), 2);
	ASSERT_EQ(sampleGraph.getArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(2ul)->getIncomingArcs().size(), 0);
	ASSERT_EQ(sampleGraph.getNode(2ul)->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(3ul)->getIncomingArcs().size(), 1);
}

TEST_F(SampleGraphTest, deleteNodeWithSelfLink) {
	sampleGraph.link(1ul, 1ul, 4ul);
	sampleGraph.removeNode(1ul);

	ASSERT_EQ(sampleGraph.getNodes().size(), 2);
	ASSERT_EQ(sampleGraph.getArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(2ul)->getIncomingArcs().size(), 0);
	ASSERT_EQ(sampleGraph.getNode(2ul)->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(3ul)->getIncomingArcs().size(), 1);

}
