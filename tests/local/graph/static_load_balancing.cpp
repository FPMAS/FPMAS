#include "../../mocks/graph/mock_load_balancing.h"
#include "fpmas/api/graph/load_balancing.h"
#include "fpmas/graph/static_load_balancing.h"
#include "graph/mock_distributed_node.h"

using namespace testing;

TEST(StaticLoadBalancing, test) {
	StrictMock<MockLoadBalancing<int>> mock_lb;
	fpmas::graph::StaticLoadBalancing<int> static_lb(mock_lb);

	fpmas::api::graph::NodeMap<int> nodes;
	for(unsigned long i = 0; i < 10; i++) {
		nodes[{0, i}] = new MockDistributedNode<int>;
	}
	EXPECT_CALL(mock_lb, balance(UnorderedElementsAreArray(nodes), fpmas::api::graph::PARTITION));

	static_lb.balance(nodes, fpmas::api::graph::PARTITION);
	for(int i = 0; i < 10; i++) {
		static_lb.balance(nodes, fpmas::api::graph::REPARTITION);
	}

	for(auto node : nodes)
		delete node.second;
}
