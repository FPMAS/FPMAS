#include "fpmas/graph/random_load_balancing.h"
#include "../../mocks/communication/mock_communication.h"
#include "../../mocks/graph/mock_distributed_node.h"

using namespace testing;

class RandomLoadBalancing : public Test {
	protected:
		static const std::size_t NUM_NODES = 100;
		fpmas::graph::NodeMap<int> nodes;
		MockMpiCommunicator<3, 4> comm;
		fpmas::graph::RandomLoadBalancing<int> random_lb {comm};

		void SetUp() override {
			for(std::size_t i = 0; i < NUM_NODES; i++) {
				auto id = fpmas::api::graph::DistributedId(1, i);
				nodes[id] = new MockDistributedNode<int, NiceMock>(id);
			}
		}

		void TearDown() override {
			for(auto node : nodes)
				delete node.second;
		}
};

TEST_F(RandomLoadBalancing, test) {
	auto partition = random_lb.balance(nodes);

	ASSERT_THAT(partition, SizeIs(NUM_NODES));

	std::unordered_map<int, std::size_t> counts_by_proc;

	for(auto item : partition)
		counts_by_proc[item.second]++;
	for(auto count : counts_by_proc)
		ASSERT_THAT(count.second, Gt(0));
}

TEST_F(RandomLoadBalancing, dynamic) {
	auto partition1 = random_lb.balance(nodes);
	auto partition2 = random_lb.balance(nodes);

	ASSERT_THAT(partition1, Not(UnorderedElementsAreArray(partition2)));
}

class FixedVerticesRandomLoadBalancing : public RandomLoadBalancing {
	protected:
	fpmas::graph::PartitionMap fixed_vertices;


	void SetUp() override {
		RandomLoadBalancing::SetUp();
		fpmas::random::mt19937 rd;
		fpmas::random::UniformIntDistribution<std::size_t> random_id(0, NUM_NODES-1);
		fpmas::random::UniformIntDistribution<int> random_rank(0, 3);

		for(std::size_t i = 0; i < 10; i++)
			fixed_vertices[fpmas::api::graph::DistributedId(1, random_id(rd))]
				= random_rank(rd);
	}
};

TEST_F(FixedVerticesRandomLoadBalancing, test_fixed) {
	auto partition = random_lb.balance(nodes, fixed_vertices);

	for(auto item : fixed_vertices)
		ASSERT_EQ(partition[item.first], item.second);

	std::unordered_map<int, std::size_t> counts_by_proc;

	for(auto item : partition)
		if(fixed_vertices.count(item.first) == 0)
			counts_by_proc[item.second]++;

	for(auto count : counts_by_proc)
		ASSERT_THAT(count.second, Gt(0));
}

TEST_F(FixedVerticesRandomLoadBalancing, fixed_dynamic) {
	auto partition1 = random_lb.balance(nodes, fixed_vertices);
	auto partition2 = random_lb.balance(nodes, fixed_vertices);

	ASSERT_THAT(partition1, Not(UnorderedElementsAreArray(partition2)));
}
