#include "gtest/gtest.h"
#include "environment/serializers.h"
#include <nlohmann/json.hpp>

#include "sample_graph.h"

using nlohmann::json;
using FPMAS::graph::Graph;
using FPMAS::graph::Node;
using FPMAS::graph::Arc;

TEST(NodeSerializer, simple_node_serialization) {

	Graph<int> g;
	Node<int>* node = g.buildNode(85250, new int(0));

	// The node to_json method will be automatically called
	json j = json{{"node", *node}};

	ASSERT_EQ("{\"node\":{\"data\":0,\"id\":85250}}", j.dump());

	delete node->getData();

}

TEST(ArcSerializer, simple_arc_serializer) {
	Graph<int> g;
	Node<int>* n1 = g.buildNode(0ul, new int(0));
	Node<int>* n2 = g.buildNode(1ul, new int(1));
	g.link(n1, n2, 0ul);

	Arc<int>* a = n1->getOutgoingArcs()[0];

	json j = *a;

	ASSERT_EQ(j.dump(), "{\"id\":0,\"link\":[0,1]}");

	delete n1->getData();
	delete n2->getData();
}

TEST(GraphSerializer, simple_graph_serialization) {

	Graph<int> g;
	Node<int>* n = g.buildNode(0ul, new int(0));

	json j = g;

	ASSERT_EQ(j.dump(), "{\"arcs\":[],\"nodes\":[{\"data\":0,\"id\":0}]}");

	delete n->getData();
}

TEST_F(SampleGraphTest, sample_graph_serialization) {

	json j = sampleGraph;

	ASSERT_EQ(j.dump(),
			"{\"arcs\":["
				"{\"id\":2,\"link\":[2,3]},"
				"{\"id\":0,\"link\":[1,2]},"
				"{\"id\":1,\"link\":[2,1]}"
				"],"
			"\"nodes\":["
				"{\"data\":3,\"id\":3},"
				"{\"data\":1,\"id\":1},"
				"{\"data\":2,\"id\":2}"
				"]}"
			);

}

TEST(GraphSerializer, simple_graph_deserialization) {
	json graph_json = R"(
		{
			"arcs":[],
			"nodes":[{"data":0,"id":0}]
		}
		)"_json;

	Graph<int> g = graph_json.get<Graph<int>>();
	ASSERT_EQ(g.getNodes().size(), 1);
	ASSERT_EQ(g.getNodes().count(0ul), 1);
	ASSERT_EQ(g.getNodes().find(0ul)->second->getId(), 0ul);
	ASSERT_EQ(*g.getNodes().find(0ul)->second->getData(), 0);

	delete g.getNodes().find(0ul)->second->getData();
}

TEST_F(SampleGraphTest, sample_graph_deserialization) {

	json graph_json = R"(
			{
			"arcs":[
				{"id":0,"link":[1,2]},
				{"id":2,"link":[2,3]},
				{"id":1,"link":[2,1]}
				],
			"nodes":[
				{"data":3,"id":3},
				{"data":1,"id":1},
				{"data":2,"id":2}
				]
			}
			)"_json;

	Graph<int> graph = graph_json.get<Graph<int>>();

	testSampleGraphStructure(&graph);

}
