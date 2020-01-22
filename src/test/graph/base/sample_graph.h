#ifndef SAMPLE_GRAPH_H
#define SAMPLE_GRAPH_H

#include "gtest/gtest.h"
#include "graph/base/graph.h"
#include "test_utils/test_utils.h"

using FPMAS::graph::base::Graph;
using FPMAS::graph::base::Node;
using FPMAS::graph::base::Arc;

using FPMAS::test_utils::assert_contains;

class SampleGraphTest : public ::testing::Test {

	protected:
		Graph<int> sampleGraph;

		void SetUp() override {
			Node<int>* node1 = sampleGraph.buildNode(1ul, 1);
			Node<int>* node2 = sampleGraph.buildNode(2ul, 2);
			Node<int>* node3 = sampleGraph.buildNode(3ul, 3);

			sampleGraph.link(node1, node2, 0ul);
			sampleGraph.link(node2, node1, 1ul);
			sampleGraph.link(node2, node3, 2ul);

		}

		static void testSampleGraphStructure(Graph<int>* sampleGraph) {
			Node<int>* node1 = sampleGraph->getNode(1ul);
			ASSERT_EQ(node1->data(), 1);
			ASSERT_EQ(node1->getIncomingArcs().size(), 1);
			ASSERT_EQ(node1->getIncomingArcs().at(0), sampleGraph->getArc(1ul));
			ASSERT_EQ(node1->getOutgoingArcs().size(), 1);
			ASSERT_EQ(node1->getOutgoingArcs().at(0), sampleGraph->getArc(0ul));

			Node<int>* node2 = sampleGraph->getNode(2ul);
			ASSERT_EQ(node2->data(), 2);
			ASSERT_EQ(node2->getIncomingArcs().size(), 1);
			ASSERT_EQ(node2->getIncomingArcs().at(0), sampleGraph->getArc(0ul));
			ASSERT_EQ(node2->getOutgoingArcs().size(), 2);

			Arc<int>* outArcs[2] = {
				node2->getOutgoingArcs().at(0),
				node2->getOutgoingArcs().at(1)
			};
			assert_contains<Arc<int>*, 2>(outArcs, sampleGraph->getArc(1ul));
			assert_contains<Arc<int>*, 2>(outArcs, sampleGraph->getArc(2ul));

			Node<int>* node3 = sampleGraph->getNode(3ul);
			ASSERT_EQ(node3->data(), 3);
			ASSERT_EQ(node3->getIncomingArcs().size(), 1);
			ASSERT_EQ(node3->getIncomingArcs().at(0), sampleGraph->getArc(2ul));
			ASSERT_EQ(node3->getOutgoingArcs().size(), 0);
		}

};
#endif
