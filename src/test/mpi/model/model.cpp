#include "model/model.h"
#include "model/serializer.h"
#include "graph/parallel/distributed_graph.h"
#include "load_balancing/zoltan_load_balancing.h"
#include "synchro/ghost/ghost_mode.h"
#include "scheduler/scheduler.h"
#include "runtime/runtime.h"
#include "../mocks/model/mock_model.h"

FPMAS_JSON_SERIALIZE_AGENT(MockAgent<1>, MockAgent<10>)

class ModelIntegrationTest : public ::testing::Test {
	protected:
		FPMAS::model::AgentGraph<FPMAS::synchro::GhostMode> agent_graph;
		FPMAS::model::ZoltanLoadBalancing lb {agent_graph.getMpiCommunicator().getMpiComm()};
		FPMAS::model::ScheduledLoadBalancing scheduled_lb {lb, scheduler, runtime};

		FPMAS::scheduler::Scheduler scheduler;
		FPMAS::runtime::Runtime runtime {scheduler};

		FPMAS::model::Model model {agent_graph, scheduler, runtime, scheduled_lb};

		void SetUp() override {
			auto& group1 = model.buildGroup();
			auto& group2 = model.buildGroup();

			if(agent_graph.getMpiCommunicator().getRank() == 0) {
				for(int i = 0; i < 10 * agent_graph.getMpiCommunicator().getSize(); i++) {
					group1.add(new MockAgent<1>);
					group2.add(new MockAgent<10>);
				}
			}
			scheduler.schedule(0, 1, group1.job());
			scheduler.schedule(0, 2, group2.job());
			scheduler.schedule(0, model.loadBalancingJob());
		}
};

TEST_F(ModelIntegrationTest, distribute) {
	runtime.run(1);
}
