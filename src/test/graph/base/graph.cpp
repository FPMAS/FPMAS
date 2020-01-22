#include "gtest/gtest.h"
#include "graph/base/graph.h"

#include "sample_graph.h"

using FPMAS::graph::base::Graph;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

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

void checkUnlinkArc(Graph<int>& sampleGraph) {
	// The arc has been deleted
	ASSERT_EQ(sampleGraph.getArcs().size(), 2);
	ASSERT_EQ(sampleGraph.getNode(2ul)->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(1ul)->getIncomingArcs().size(), 0);

	// The rest of the structure keeps unchanged
	ASSERT_EQ(sampleGraph.getNode(2ul)->getIncomingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(2ul)->getIncomingArcs().at(0)->getId(), 0ul);
	ASSERT_EQ(sampleGraph.getNode(2ul)->getOutgoingArcs().at(0)->getId(), 2ul);

	ASSERT_EQ(sampleGraph.getNode(1ul)->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(1ul)->getOutgoingArcs().at(0)->getId(), 0ul);

}

TEST_F(SampleGraphTest, unlinkArcTest) {
	sampleGraph.unlink(sampleGraph.getArc(1ul));
	checkUnlinkArc(sampleGraph);
}

TEST_F(SampleGraphTest, unlinkArcById) {
	sampleGraph.unlink(1ul);
	checkUnlinkArc(sampleGraph);
}


