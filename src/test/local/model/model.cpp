#include "model/model.h"

#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/mock_load_balancing.h"
#include "../mocks/model/mock_model.h"

using ::testing::Eq;
using ::testing::Property;
using ::testing::Ref;
using ::testing::UnorderedElementsAre;

using FPMAS::model::Model;
using FPMAS::model::AgentGroup;

typedef FPMAS::api::model::Agent* AgentPtr;

class ModelTest : public ::testing::Test {
	public:
		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
			graph;
		MockLoadBalancing<AgentPtr> load_balancing;

		Model model {graph, load_balancing};
};

TEST_F(ModelTest, graph) {
	ASSERT_THAT(model.graph(), Ref(graph));
};

TEST_F(ModelTest, buildGroup) {
	auto group_1 = model.buildGroup();
	auto group_2 = model.buildGroup();

	ASSERT_THAT(model.groups(), UnorderedElementsAre(group_1, group_2));
}

class AgentGroupTest : public ::testing::Test {
	protected:
		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
			graph;
		AgentGroup agent_group {graph, FPMAS::JID(18)};

};

TEST_F(AgentGroupTest, job) {
	ASSERT_EQ(agent_group.job().id(), FPMAS::JID(18));
}

TEST_F(AgentGroupTest, job_end) {
	EXPECT_CALL(graph, synchronize);

	agent_group.job().getEndTask().run();
}

TEST_F(AgentGroupTest, add_agent) {
	MockAgent* agent = new MockAgent;


	MockDistributedNode<AgentPtr> node;
	EXPECT_CALL(graph, buildNode(Ref(agent)))
		.WillOnce(Return(&node));
	EXPECT_CALL(*agent, setNode(&node));

	agent_group.add(agent);
}

TEST_F(AgentGroupTest, agent_task) {

}
