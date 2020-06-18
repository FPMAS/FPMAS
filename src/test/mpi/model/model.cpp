#include "model/model.h"
#include "model/serializer.h"
#include "graph/parallel/distributed_graph.h"
#include "load_balancing/zoltan_load_balancing.h"
#include "synchro/ghost/ghost_mode.h"
#include "scheduler/scheduler.h"
#include "runtime/runtime.h"
#include "../mocks/model/mock_model.h"

#include <random>
#include <numeric>

using ::testing::SizeIs;
using ::testing::Ge;
using ::testing::InvokeWithoutArgs;

FPMAS_JSON_SERIALIZE_AGENT(MockAgentBase<1>, MockAgentBase<10>)
FPMAS_DEFAULT_JSON(MockAgentBase<1>)
FPMAS_DEFAULT_JSON(MockAgentBase<10>)

class ModelIntegrationTest : public ::testing::Test {
	protected:
		inline static const int NODE_BY_PROC = 50;
		inline static const int NUM_STEPS = 100;
		FPMAS::model::AgentGraph<FPMAS::synchro::GhostMode> agent_graph;
		FPMAS::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};

		FPMAS::scheduler::Scheduler scheduler;
		FPMAS::runtime::Runtime runtime {scheduler};
		FPMAS::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		FPMAS::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		std::unordered_map<DistributedId, unsigned long> act_counts;
		std::unordered_map<DistributedId, FPMAS::api::model::TypeId> agent_types;

};

class IncreaseCount {
	private:
		std::unordered_map<DistributedId, unsigned long>& act_counts;
		DistributedId id;
	public:
		IncreaseCount(std::unordered_map<DistributedId, unsigned long>& act_counts, DistributedId id)
			: act_counts(act_counts), id(id) {}
		void operator()() {
			act_counts[id]++;
		}
};

class IncreaseCountAct : public FPMAS::model::AgentNodeCallback {
	private:
		std::unordered_map<DistributedId, unsigned long>& act_counts;
	public:
		IncreaseCountAct(std::unordered_map<DistributedId, unsigned long>& act_counts) : act_counts(act_counts) {}

		void call(FPMAS::model::AgentNode* node) override {
			switch(node->data().get()->typeId()) {
				case MockAgentBase<1>::TYPE_ID :
					EXPECT_CALL(*static_cast<MockAgentBase<1>*>(node->data().get()), act)
						.Times(AnyNumber())
						.WillRepeatedly(InvokeWithoutArgs(IncreaseCount(act_counts, node->getId())));
					break;
				case MockAgentBase<10>::TYPE_ID :
					EXPECT_CALL(*static_cast<MockAgentBase<10>*>(node->data().get()), act)
						.Times(AnyNumber())
						.WillRepeatedly(InvokeWithoutArgs(IncreaseCount(act_counts, node->getId())));
					break;
				default:
					break;
			}
		}
};

class ModelIntegrationExecutionTest : public ModelIntegrationTest {
	protected:
		void SetUp() override {
			auto& group1 = model.buildGroup();
			auto& group2 = model.buildGroup();

			agent_graph.addCallOnSetLocal(new IncreaseCountAct(act_counts));
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new MockAgentBase<1>);
					group2.add(new MockAgentBase<10>);
				}
			}
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				act_counts[{0, (unsigned int) id}] = 0;
				if(id % 2 == 0) {
					agent_types[{0, (unsigned int) id}] = 1;
				} else {
					agent_types[{0, (unsigned int) id}] = 10;
				}
			}
		}

		void checkExecutionCounts() {
			FPMAS::communication::TypedMpi<int> mpi {agent_graph.getMpiCommunicator()};
			// TODO : MPI gather to compute the sum of act counts.
			for(int id = 0; id < 2 * NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); id++) {
				DistributedId dist_id {0, (unsigned int) id};
				std::vector<int> counts = mpi.gather(act_counts[dist_id], 0);
				if(agent_graph.getMpiCommunicator().getRank() == 0) {
					int sum = 0;
					sum = std::accumulate(counts.begin(), counts.end(), sum);
					if(agent_types[dist_id] == 1) {
						// Executed every step
						ASSERT_EQ(sum, NUM_STEPS);
					} else {
						// Executed one in two step
						ASSERT_EQ(sum, NUM_STEPS / 2);
					}
				}
			}
		}
};

TEST_F(ModelIntegrationExecutionTest, ghost_mode) {
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

TEST_F(ModelIntegrationExecutionTest, ghost_mode_with_link) {
	std::mt19937 engine;
	std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
	std::uniform_int_distribution<unsigned int> random_layer {0, 10};

	for(auto node : agent_graph.getNodes()) {
		for(int i = 0; i < NODE_BY_PROC / 2; i++) {
			DistributedId id {0, random_node(engine)};
			if(node.first.id() != id.id())
				agent_graph.link(node.second, agent_graph.getNode(id), 10);
		}
	}
	scheduler.schedule(0, model.loadBalancingJob());
	runtime.run(NUM_STEPS);
	checkExecutionCounts();
}

class UpdateWeightAct : public FPMAS::model::AgentNodeCallback {
	private:
		std::mt19937 engine;
		std::uniform_real_distribution<float> random_node {0, 1};
	public:
		void call(FPMAS::model::AgentNode* node) override {
			node->setWeight(random_node(engine) * 10);
			EXPECT_CALL(*static_cast<MockAgentBase<10>*>(node->data().get()), act)
				.Times(AnyNumber());
		}
};

class ModelIntegrationLoadBalancingTest : public ModelIntegrationTest {
	protected:
		void SetUp() override {
			auto& group1 = model.buildGroup();
			auto& group2 = model.buildGroup();

			agent_graph.addCallOnSetLocal(new UpdateWeightAct());
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group2.add(new MockAgentBase<10>);
				}
			}
			std::mt19937 engine;
			std::uniform_int_distribution<unsigned int> random_node {0, (unsigned int) agent_graph.getNodes().size()-1};
			std::uniform_int_distribution<unsigned int> random_layer {0, 10};

			for(auto node : agent_graph.getNodes()) {
				for(int i = 0; i < NODE_BY_PROC / 2; i++) {
					DistributedId id {0, random_node(engine)};
					if(node.first.id() != id.id())
						agent_graph.link(node.second, agent_graph.getNode(id), 10);
				}
			}
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			scheduler.schedule(0, 4, model.loadBalancingJob());
		}
};

TEST_F(ModelIntegrationLoadBalancingTest, ghost_mode_dynamic_lb) {
	runtime.run(NUM_STEPS);
}
