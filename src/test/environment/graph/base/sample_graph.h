#ifndef SAMPLE_GRAPH_H
#define SAMPLE_GRAPH_H

#include "gtest/gtest.h"
#include "environment/graph/base/graph.h"
#include "test_utils/test_utils.h"

using FPMAS::graph::Graph;
using FPMAS::graph::Node;
using FPMAS::graph::Arc;

using FPMAS::test_utils::assert_contains;

class SampleGraphTest : public ::testing::Test {

	protected:
		Graph<int> sampleGraph;
		std::vector<int*> data = {
			new int(1),
			new int(2),
			new int(3)
		};

		void SetUp() override {
			Node<int>* node1 = sampleGraph.buildNode(1ul, data[0]);
			Node<int>* node2 = sampleGraph.buildNode(2ul, data[1]);
			Node<int>* node3 = sampleGraph.buildNode(3ul, data[2]);

			sampleGraph.link(node1, node2, 0ul);
			sampleGraph.link(node2, node1, 1ul);
			sampleGraph.link(node2, node3, 2ul);

		}

		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}

		static void testSampleGraphStructure(Graph<int>* sampleGraph) {
			Node<int>* node1 = sampleGraph->getNode(1ul);
			ASSERT_EQ(*node1->getData(), 1);
			ASSERT_EQ(node1->getIncomingArcs().size(), 1);
			ASSERT_EQ(node1->getIncomingArcs().at(0), sampleGraph->getArc(1ul));
			ASSERT_EQ(node1->getOutgoingArcs().size(), 1);
			ASSERT_EQ(node1->getOutgoingArcs().at(0), sampleGraph->getArc(0ul));

			Node<int>* node2 = sampleGraph->getNode(2ul);
			ASSERT_EQ(*node2->getData(), 2);
			ASSERT_EQ(node2->getIncomingArcs().size(), 1);
			ASSERT_EQ(node2->getIncomingArcs().at(0), sampleGraph->getArc(0ul));
			ASSERT_EQ(node2->getOutgoingArcs().size(), 2);
			assert_contains<Arc<int>*>(node2->getOutgoingArcs(), sampleGraph->getArc(1ul));
			assert_contains<Arc<int>*>(node2->getOutgoingArcs(), sampleGraph->getArc(2ul));

			Node<int>* node3 = sampleGraph->getNode(3ul);
			ASSERT_EQ(*node3->getData(), 3);
			ASSERT_EQ(node3->getIncomingArcs().size(), 1);
			ASSERT_EQ(node3->getIncomingArcs().at(0), sampleGraph->getArc(2ul));
			ASSERT_EQ(node3->getOutgoingArcs().size(), 0);
		}

};
#endif
