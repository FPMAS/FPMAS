#include "gtest/gtest.h"
#include "environment/graph.h"

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
