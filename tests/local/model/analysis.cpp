#include "../../mocks/model/mock_model.h"
#include "fpmas/model/analysis.h"

using namespace testing;

TEST(LocalAgentIds, test) {
	NiceMock<MockAgentGroup> mock_group;
	std::vector<fpmas::api::graph::DistributedId> agent_ids;
	std::vector<fpmas::api::model::Agent*> mock_agents;
	std::vector<fpmas::api::model::AgentNode*> mock_nodes;
	for(std::size_t i = 0; i < 10; i++) {
		auto mock_agent = new MockAgent<>;
		auto mock_node = new MockDistributedNode<fpmas::api::model::AgentPtr, testing::NiceMock>({0, i});
		agent_ids.push_back({0, i});
		mock_agents.push_back(mock_agent);
		mock_nodes.push_back(mock_node);
		ON_CALL(*mock_agent, node())
			.WillByDefault(Return(mock_node));
	}
	ON_CALL(mock_group, localAgents)
		.WillByDefault(Return(mock_agents));

	ASSERT_THAT(
			fpmas::model::local_agent_ids(mock_group),
			UnorderedElementsAreArray(agent_ids));

	for(auto node : mock_nodes)
		delete node;
	for(auto agent : mock_agents)
		delete agent;
}
