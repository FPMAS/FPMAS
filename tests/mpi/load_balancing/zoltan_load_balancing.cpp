#include "fpmas/graph/parallel/distributed_graph.h"
#include "fpmas/load_balancing/zoltan_load_balancing.h"
#include "fpmas/synchro/ghost/ghost_mode.h"

#include <random>
#include "gmock/gmock.h"

using ::testing::SizeIs;
using ::testing::Ge;
using ::testing::Contains;
using ::testing::Pair;
using ::testing::_;

using fpmas::graph::parallel::DistributedGraph;
using fpmas::load_balancing::ZoltanLoadBalancing;

class ZoltanLoadBalancingIntegrationTest : public ::testing::Test {
	protected:
		DistributedGraph<
			int,
			fpmas::synchro::GhostMode,
			fpmas::graph::parallel::DistributedNode,
			fpmas::graph::parallel::DistributedEdge,
			fpmas::api::communication::MpiSetUp<
				fpmas::communication::MpiCommunicator,
				fpmas::communication::TypedMpi>,
			fpmas::graph::parallel::LocationManager
				> graph;

		ZoltanLoadBalancing<int> load_balancing {graph.getMpiCommunicator().getMpiComm()};
		fpmas::load_balancing::zoltan::PartitionMap<int> fixed_nodes;

		void SetUp() override {
			if(graph.getMpiCommunicator().getRank() == 0) {
				for (int i = 0; i < 10*graph.getMpiCommunicator().getSize(); ++i) {
					graph.buildNode();
				}	
				std::mt19937 engine;
				std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) graph.getNodes().size()-1};
				for(auto node : graph.getNodes()) {
					for(int i = 0; i < 4; i++) {
						graph.link(node.second, graph.getNode({0, random_node(engine)}), 0);

					}
				}

				std::uniform_int_distribution<int> random_rank {0, graph.getMpiCommunicator().getSize()-1};
				for(int i = 0; i < 10; i++) {
					unsigned int rand = random_node(engine);
					fixed_nodes[{0, rand}] = random_rank(engine);
				}
			}
		}
};

TEST_F(ZoltanLoadBalancingIntegrationTest, balance) {
	typename ZoltanLoadBalancing<int>::ConstNodeMap map;
	for(auto node : graph.getNodes()) {
		map.insert(node);
	}
	auto partition = load_balancing.balance(map, fixed_nodes);
	for(auto fixed_node : fixed_nodes) {
		ASSERT_EQ(partition[fixed_node.first], fixed_node.second);
	}
}

TEST_F(ZoltanLoadBalancingIntegrationTest, graph_balance) {
	graph.balance(load_balancing, fixed_nodes);

	ASSERT_THAT(graph.getNodes(), SizeIs(Ge(1)));
	for(auto fixed_node : fixed_nodes) {
		if(fixed_node.second == graph.getMpiCommunicator().getRank()) {
			ASSERT_THAT(graph.getNodes(), Contains(Pair(fixed_node.first, _)));
		}
	}
}
