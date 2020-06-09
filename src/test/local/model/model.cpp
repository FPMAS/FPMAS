#include "model/model.h"

#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/graph/parallel/mock_load_balancing.h"

using ::testing::Eq;
using ::testing::Ref;
using ::testing::UnorderedElementsAre;

using FPMAS::model::Model;
using FPMAS::model::AgentGroup;

typedef std::unique_ptr<FPMAS::api::model::Agent*> AgentPtr;

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
