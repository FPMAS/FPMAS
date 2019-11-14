#include "gtest/gtest.h"
#include "environment/serializers.h"
#include <nlohmann/json.hpp>

using nlohmann::json;
using FPMAS::graph::Graph;
using FPMAS::graph::Node;

TEST(NodeSerializer, simple_node_serialization) {

	Graph<int> g;
	Node<int>* node = g.buildNode("fake", new int(0));

	json j = json{{"node", *node}};

	ASSERT_EQ("{\"node\":{\"data\":0,\"label\":\"fake\"}}", j.dump());

}
