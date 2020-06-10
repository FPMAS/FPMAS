#include "model/model.h"

#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/model/mock_model.h"

using ::testing::An;
using ::testing::Eq;
using ::testing::ElementsAre;
using ::testing::Property;
using ::testing::Ref;
using ::testing::SizeIs;
using ::testing::TypedEq;
using ::testing::UnorderedElementsAre;
using ::testing::WhenDynamicCastTo;

using FPMAS::model::Model;
using FPMAS::model::AgentGroup;
using FPMAS::model::AgentTask;

typedef FPMAS::api::model::Agent* AgentPtr;

TEST(AgentTaskTest, build) {
	MockAgent agent;
	MockDistributedNode<AgentPtr> node;

	AgentTask agent_task {agent};
	ON_CALL(agent, node())
		.WillByDefault(Return(&node));
	EXPECT_CALL(agent, node()).Times(AnyNumber());
	ON_CALL(Const(agent), node())
		.WillByDefault(Return(&node));
	EXPECT_CALL(Const(agent), node()).Times(AnyNumber());

	ASSERT_THAT(agent_task.agent(), Ref(agent));
	ASSERT_THAT(agent_task.node(), Eq(&node));
}

TEST(AgentTaskTest, run) {
	MockAgent agent;
	AgentTask agent_task {agent};

	EXPECT_CALL(agent, act);

	agent_task.run();

}

class ModelTest : public ::testing::Test {
	public:
		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
			graph;
		MockLoadBalancing<AgentPtr> load_balancing;

		FPMAS::scheduler::Scheduler scheduler;
		FPMAS::runtime::Runtime runtime {scheduler};

		Model model {graph, load_balancing};
};

TEST_F(ModelTest, graph) {
	ASSERT_THAT(model.graph(), Ref(graph));
}

TEST_F(ModelTest, buildGroup) {
	auto group_1 = model.buildGroup();
	auto group_2 = model.buildGroup();

	ASSERT_THAT(model.groups(), UnorderedElementsAre(group_1, group_2));
}

TEST_F(ModelTest, load_balancing_job) {
	scheduler.schedule(0, model.loadBalancingJob());

	EXPECT_CALL(graph, balance(Ref(load_balancing)));
	runtime.run(1);
}

class AgentGroupTest : public ::testing::Test {
	protected:
		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
			graph;
		AgentGroup agent_group {graph, FPMAS::JID(18)};

		FPMAS::scheduler::Scheduler scheduler;
		FPMAS::runtime::Runtime runtime {scheduler};
};

TEST_F(AgentGroupTest, job) {
	ASSERT_EQ(agent_group.job().id(), FPMAS::JID(18));
}

TEST_F(AgentGroupTest, job_end) {
	EXPECT_CALL(graph, synchronize);

	agent_group.job().getEndTask().run();
}

TEST_F(AgentGroupTest, add_agent) {
	MockAgent* agent1 = new MockAgent;
	MockAgent* agent2 = new MockAgent;

	// Agent 1 set up
	MockDistributedNode<AgentPtr> node1;
	EXPECT_CALL(graph, buildNode(TypedEq<FPMAS::api::model::Agent*const&>(agent1)))
		.WillOnce(Return(&node1));
	EXPECT_CALL(*agent1, setNode(&node1));

	agent_group.add(agent1);

	// Agent 2 set up
	MockDistributedNode<AgentPtr> node2;
	EXPECT_CALL(graph, buildNode(TypedEq<FPMAS::api::model::Agent*const&>(agent2)))
		.WillOnce(Return(&node2));
	EXPECT_CALL(*agent2, setNode(&node2));

	agent_group.add(agent2);

	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(agent1, agent2));
}

TEST_F(AgentGroupTest, agent_task) {
	MockAgent* agent1 = new MockAgent;
	MockAgent* agent2 = new MockAgent;

	MockDistributedNode<AgentPtr> node;
	EXPECT_CALL(graph, buildNode(An<FPMAS::api::model::Agent* const&>()))
		.Times(2)
		.WillRepeatedly(Return(&node));
	EXPECT_CALL(*agent1, setNode);
	EXPECT_CALL(*agent2, setNode);

	agent_group.add(agent1);
	agent_group.add(agent2);

	ASSERT_THAT(agent_group.job().tasks(), SizeIs(2));

	EXPECT_CALL(graph, synchronize);
	::testing::Sequence s1, s2;
	EXPECT_CALL(*agent1, act)
		.InSequence(s1);
	EXPECT_CALL(*agent2, act)
		.InSequence(s2);

	scheduler.schedule(0, agent_group.job());
	runtime.run(1);
}
