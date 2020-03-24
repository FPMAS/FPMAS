#include "gtest/gtest.h"

#include "graph/base/exceptions.h"
#include "graph/base/graph.h"

#include "sample_graph.h"

using FPMAS::graph::base::Graph;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

/*
 * Trivial constructor test.
 */
TEST(GraphTest, buildTest) {
	Graph<int> graph = Graph<int>(0ul);
}

TEST_F(SampleGraphTest, buildSimpleGraph) {
	testSampleGraphStructure(&sampleGraph);
}

TEST_F(SampleGraphTest, deleteNodeTest) {
	sampleGraph.removeNode(DefaultId(0ul));

	ASSERT_EQ(sampleGraph.getNodes().size(), 2);
	ASSERT_EQ(sampleGraph.getArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getIncomingArcs().size(), 0);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(2ul))->getIncomingArcs().size(), 1);
}

TEST_F(SampleGraphTest, deleteNodeWithSelfLink) {
	sampleGraph.link(DefaultId(0ul), DefaultId(0ul));
	sampleGraph.removeNode(DefaultId(0ul));

	ASSERT_EQ(sampleGraph.getNodes().size(), 2);
	ASSERT_EQ(sampleGraph.getArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getIncomingArcs().size(), 0);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(2ul))->getIncomingArcs().size(), 1);

}

void checkUnlinkArc(Graph<int>& sampleGraph) {
	// The arc has been deleted
	ASSERT_EQ(sampleGraph.getArcs().size(), 2);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(0ul))->getIncomingArcs().size(), 0);

	// The rest of the structure keeps unchanged
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getIncomingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getIncomingArcs().at(0)->getId(), 0ul);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(1ul))->getOutgoingArcs().at(0)->getId(), 2ul);

	ASSERT_EQ(sampleGraph.getNode(DefaultId(0ul))->getOutgoingArcs().size(), 1);
	ASSERT_EQ(sampleGraph.getNode(DefaultId(0ul))->getOutgoingArcs().at(0)->getId(), 0ul);

}

TEST_F(SampleGraphTest, unlinkArcTest) {
	sampleGraph.unlink(sampleGraph.getArc(DefaultId(1ul)));
	checkUnlinkArc(sampleGraph);
}

TEST_F(SampleGraphTest, unlinkArcById) {
	sampleGraph.unlink(DefaultId(1ul));
	checkUnlinkArc(sampleGraph);
}

using FPMAS::graph::base::exceptions::arc_out_of_graph;

TEST(GraphExceptions, unlink_exception) {
	Graph<int> graph_1(0);
	Graph<int> graph_2(0);

	auto arc = graph_1.link(
		graph_1.buildNode(),
		graph_1.buildNode()
		);

	ASSERT_THROW(graph_2.unlink(arc), arc_out_of_graph<DefaultId>);
	ASSERT_THROW(graph_2.unlink(arc->getId()), arc_out_of_graph<DefaultId>);

}
