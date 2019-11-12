#include "gtest/gtest.h"
#include "environment/graph.h"

/*
 * Trivial constructor test.
 */
TEST(GraphTest, buildTest) {
	Graph<int> graph = Graph<int>();
}

class SimpleGraphTest : public ::testing::Test {

	protected:
		Graph<int> simpleGraph;
		std::vector<int*> data = {
			new int(1),
			new int(2),
			new int(3)
		};

		void SetUp() override {
			Node<int>* node1 = simpleGraph.buildNode("1", data[0]);
			Node<int>* node2 = simpleGraph.buildNode("2", data[1]);
			Node<int>* node3 = simpleGraph.buildNode("3", data[2]);

			simpleGraph.link(node1, node2, "0");
			simpleGraph.link(node2, node1, "1");
			simpleGraph.link(node2, node3, "2");

		}

		void TearDown() override {
			for(auto d : data) {
				delete d;
			}
		}

};

bool contains(std::vector<Arc<int>*> v, Arc<int>* a) {
	for(auto _v : v) {
		if(_v == a) {
			return true;
		}
	}
	return false;
}

TEST_F(SimpleGraphTest, buildSimpleGraph) {

	Node<int>* node1 = simpleGraph.getNode("1");
	ASSERT_EQ(*node1->getData(), 1);
	ASSERT_EQ(node1->getIncomingArcs().size(), 1);
	ASSERT_EQ(node1->getIncomingArcs().at(0), simpleGraph.getArc("1"));
	ASSERT_EQ(node1->getOutgoingArcs().size(), 1);
	ASSERT_EQ(node1->getOutgoingArcs().at(0), simpleGraph.getArc("0"));

	Node<int>* node2 = simpleGraph.getNode("2");
	ASSERT_EQ(*node2->getData(), 2);
	ASSERT_EQ(node2->getIncomingArcs().size(), 1);
	ASSERT_EQ(node2->getIncomingArcs().at(0), simpleGraph.getArc("0"));
	ASSERT_EQ(node2->getOutgoingArcs().size(), 2);
	ASSERT_TRUE(contains(node2->getOutgoingArcs(), simpleGraph.getArc("1")));
	ASSERT_TRUE(contains(node2->getOutgoingArcs(), simpleGraph.getArc("2")));

	Node<int>* node3 = simpleGraph.getNode("3");
	ASSERT_EQ(*node3->getData(), 3);
	ASSERT_EQ(node3->getIncomingArcs().size(), 1);
	ASSERT_EQ(node3->getIncomingArcs().at(0), simpleGraph.getArc("2"));
	ASSERT_EQ(node3->getOutgoingArcs().size(), 0);


}
