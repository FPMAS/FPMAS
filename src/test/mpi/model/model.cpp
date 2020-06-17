#include "model/model.h"
#include "model/serializer.h"
#include "graph/parallel/distributed_graph.h"
#include "load_balancing/zoltan_load_balancing.h"
#include "synchro/ghost/ghost_mode.h"
#include "scheduler/scheduler.h"
#include "runtime/runtime.h"
#include "../mocks/model/mock_model.h"

#include <random>

using ::testing::SizeIs;
using ::testing::Ge;
using ::testing::InvokeWithoutArgs;

FPMAS_JSON_SERIALIZE_AGENT(MockAgentBase<1>, MockAgentBase<10>)
FPMAS_DEFAULT_JSON(MockAgentBase<1>)
FPMAS_DEFAULT_JSON(MockAgentBase<10>)

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

class ExpectAct : public FPMAS::model::AgentNodeCallback {
	private:
		std::unordered_map<DistributedId, unsigned long>& act_counts;
	public:
		ExpectAct(std::unordered_map<DistributedId, unsigned long>& act_counts) : act_counts(act_counts) {}

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

class ModelIntegrationTest : public ::testing::Test {
	protected:
		inline static const int NODE_BY_PROC = 50;
		FPMAS::model::AgentGraph<FPMAS::synchro::GhostMode> agent_graph;
		FPMAS::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};

		FPMAS::scheduler::Scheduler scheduler;
		FPMAS::runtime::Runtime runtime {scheduler};
		FPMAS::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		FPMAS::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		std::unordered_map<DistributedId, unsigned long> act_counts;

		void SetUp() override {
			auto& group1 = model.buildGroup();
			auto& group2 = model.buildGroup();

			agent_graph.addCallOnSetLocal(new ExpectAct(act_counts));
			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < NODE_BY_PROC * agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new MockAgentBase<1>);
					group2.add(new MockAgentBase<10>);
				}
			}
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			scheduler.schedule(0, model.loadBalancingJob());
		}
};

TEST_F(ModelIntegrationTest, distribute) {
	//ASSERT_THAT(agent_graph.getNodes(), SizeIs(Ge(1)));
	//FPMAS_LOGI(
			//agent_graph.getMpiCommunicator().getRank(), "MODEL_TEST",
			//"Executing %lu agents for 100 steps.", agent_graph.getNodes().size());
	runtime.run(100);

   /* agent_graph.getNodes();*/
	//// TODO : MPI gather to compute the sum of act counts.
	//for(auto node : agent_graph.getNodes()) {
		//auto agent = node.second->data().get();
		//if(dynamic_cast<MockAgentBase<1>*>(agent)) {
			//ASSERT_EQ(act_counts[node.first], 100);
		//} else {
			//ASSERT_EQ(act_counts[node.first], 50);
		//}
   /* }*/
}

TEST_F(ModelIntegrationTest, distribute_with_link) {
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
	runtime.run(100);
}
