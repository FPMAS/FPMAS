#include "fpmas/graph/distributed_graph.h"
#include "fpmas/graph/zoltan_load_balancing.h"
#include "fpmas/synchro/ghost/ghost_mode.h"

#include <random>
#include "gmock/gmock.h"

using namespace testing;

using fpmas::graph::DistributedGraph;
using fpmas::graph::ZoltanLoadBalancing;

class ZoltanLoadBalancingIntegrationTest : public Test {
	protected:
		fpmas::communication::MpiCommunicator comm;
		DistributedGraph<int, fpmas::synchro::GhostMode> graph {comm};

		ZoltanLoadBalancing<int> load_balancing {comm};
		fpmas::graph::PartitionMap fixed_nodes;

		void SetUp() override {
			FPMAS_ON_PROC(comm, 0) {
				for (int i = 0; i < 10*graph.getMpiCommunicator().getSize(); ++i) {
					graph.buildNode();
				}	
				std::mt19937 engine;
				std::uniform_int_distribution<FPMAS_ID_TYPE> random_node(
						0, (FPMAS_ID_TYPE) graph.getNodes().size()-1);
				for(auto node : graph.getNodes()) {
					for(int i = 0; i < 4; i++) {
						graph.link(node.second, graph.getNode({0, random_node(engine)}), 0);
					}
				}

				std::uniform_int_distribution<int> random_rank {0, graph.getMpiCommunicator().getSize()-1};
				for(int i = 0; i < 10; i++) {
					fixed_nodes[{0, random_node(engine)}] = random_rank(engine);
				}
			}
		}
};

TEST_F(ZoltanLoadBalancingIntegrationTest, balance) {
	fpmas::api::graph::NodeMap<int> map;
	for(auto node : graph.getNodes()) {
		map.insert(node);
	}
	auto partition = load_balancing.balance(map, fixed_nodes, fpmas::api::graph::PARTITION);
	for(auto fixed_node : fixed_nodes) {
		ASSERT_EQ(partition[fixed_node.first], fixed_node.second);
	}
}

TEST_F(ZoltanLoadBalancingIntegrationTest, graph_balance) {
	graph.balance(load_balancing, fixed_nodes, fpmas::api::graph::PARTITION);

	ASSERT_THAT(graph.getNodes(), SizeIs(Ge(1)));
	for(auto fixed_node : fixed_nodes) {
		if(fixed_node.second == graph.getMpiCommunicator().getRank()) {
			ASSERT_THAT(graph.getNodes(), Contains(Pair(fixed_node.first, _)));
		}
	}
}
