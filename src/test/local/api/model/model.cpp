#include "api/model/types.h"
#include "api/model/model.h"

using fpmas::api::model::AgentPtrWrapper;

class AgentPtrWrapperTest : public ::testing::Test {
	

};

TEST_F(AgentPtrWrapperTest, copy) {
	AgentPtrWrapper ptr_wrapper {new MockAgent<4>};
	AgentPtrWrapper other {ptr_wrapper};

}
