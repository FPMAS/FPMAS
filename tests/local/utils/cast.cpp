#include "fpmas/utils/cast.h"
#include "../../mocks/model/mock_model.h"

using namespace testing;

TEST(ptr_wrapper_cast, cast_agent) {
	MockAgent<> mock_agent;

	fpmas::api::model::WeakAgentPtr weak_ptr
		= fpmas::utils::ptr_wrapper_cast<fpmas::api::model::Agent>(&mock_agent);

	ASSERT_THAT(weak_ptr, Not(IsNull()));
}

TEST(ptr_wrapper_cast, cast_agent_vector) {
	std::vector<MockAgent<>*> mock_agents;
	for(std::size_t i = 0; i < 10; i++)
		mock_agents.push_back(new MockAgent<>);

	std::vector<fpmas::api::model::WeakAgentPtr> weak_ptrs
		= fpmas::utils::ptr_wrapper_cast<fpmas::api::model::Agent>(mock_agents);

	ASSERT_THAT(weak_ptrs, Each(Not(IsNull())));

	for(auto agent : mock_agents)
		delete agent;
}
