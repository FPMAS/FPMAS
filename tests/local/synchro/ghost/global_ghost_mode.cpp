#include "fpmas/synchro/ghost/global_ghost_mode.h"
#include "fpmas/model/model.h"

#include "gmock/gmock.h"

using namespace testing;
using fpmas::synchro::ghost::GlobalGhostMutex;

TEST(GlobalGhostMutex, init) {
	int data = 8;
	GlobalGhostMutex<int> mutex(data);

	ASSERT_EQ(mutex.read(), 8);
	ASSERT_EQ(mutex.acquire(), 8);
}

TEST(GlobalGhostMutex, ghost_data) {
	int data = 8;
	GlobalGhostMutex<int> mutex(data);

	data = 16;

	ASSERT_EQ(mutex.read(), 8);
	ASSERT_EQ(mutex.acquire(), 8);

	mutex.synchronize();

	ASSERT_EQ(mutex.read(), 16);
	ASSERT_EQ(mutex.acquire(), 16);
}

class Agent : public fpmas::model::AgentBase<Agent> {
	public:
		int data;
};

TEST(GlobalGhostMutex, agent_ghost_data) {
	fpmas::model::AgentPtr agent = new Agent;
	static_cast<Agent*>(agent.get())->data = 12;

	GlobalGhostMutex<fpmas::model::AgentPtr> mutex(agent);

	static_cast<Agent*>(agent.get())->data = 14;

	ASSERT_EQ(static_cast<const Agent*>(mutex.read().get())->data, 12);
	ASSERT_EQ(static_cast<Agent*>(mutex.acquire().get())->data, 12);

	mutex.synchronize();

	ASSERT_EQ(static_cast<const Agent*>(mutex.read().get())->data, 14);
	ASSERT_EQ(static_cast<Agent*>(mutex.acquire().get())->data, 14);
}
