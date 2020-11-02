#include "../mocks/model/mock_model.h"

using namespace testing;

TEST(DefaultBehavior, execute) {
	fpmas::model::DefaultBehavior behavior;

	DefaultMockAgentBase<0> agent;

	EXPECT_CALL(agent, act);

	behavior.execute(&agent);
}

#define DEFAULT_COPY_MOVE(CLASS_NAME) \
		CLASS_NAME() {} \
		CLASS_NAME(const CLASS_NAME&) {} \
		CLASS_NAME& operator=(const CLASS_NAME&) { \
			return *this;\
		}\
		CLASS_NAME& operator=(CLASS_NAME&&) {\
			return *this;\
		}

namespace api {
	class Action1 {
		public:
			virtual void action_1() = 0;
	};

	class Action2 {
		public:
			virtual void action_2() = 0;
	};
}

class Agent1 : public fpmas::model::AgentBase<Agent1>, public api::Action1 {
	public:
		DEFAULT_COPY_MOVE(Agent1)

		MOCK_METHOD(void, action_1, (), (override));
		MOCK_METHOD(void, act, (), (override));
};
class Agent2 : public fpmas::model::AgentBase<Agent2>, public api::Action2 {
	public:
		DEFAULT_COPY_MOVE(Agent2)

		MOCK_METHOD(void, action_2, (), (override));
		MOCK_METHOD(void, act, (), (override));
};
class Agent1_2 : public fpmas::model::AgentBase<Agent1_2>, public api::Action1, public api::Action2 {
	public:
		DEFAULT_COPY_MOVE(Agent1_2)
		MOCK_METHOD(void, action_1, (), (override));
		MOCK_METHOD(void, action_2, (), (override));
		MOCK_METHOD(void, act, (), (override));
};

TEST(Behavior, execute) {
	auto agent_1_behavior = fpmas::model::make_behavior(&api::Action1::action_1);

	Agent1 agent_1;
	EXPECT_CALL(agent_1, action_1);

	agent_1_behavior.execute(&agent_1);
}

class AgentGroupBehaviorTest : public testing::Test {
	public:
		NiceMock<MockAgentGraph<>> graph;
		fpmas::model::Behavior<api::Action1> behavior_1 {&api::Action1::action_1};
		fpmas::model::detail::AgentGroup group_1 {0, graph, behavior_1};
		fpmas::model::Behavior<api::Action2> behavior_2 {&api::Action2::action_2};
		fpmas::model::detail::AgentGroup group_2 {1, graph, behavior_2};

		NiceMock<MockModel> model;
		fpmas::model::detail::InsertAgentNodeCallback insert_callback {model};
		fpmas::model::detail::SetAgentLocalCallback set_local_callback {model};
		fpmas::model::detail::EraseAgentNodeCallback erase_node_callback {model};

		StrictMock<Agent1> agent_1;
		fpmas::model::AgentPtr ptr_1 {&agent_1};
		NiceMock<MockAgentNode> node_1;
		StrictMock<Agent2> agent_2;
		fpmas::model::AgentPtr ptr_2 {&agent_2};
		NiceMock<MockAgentNode> node_2;
		StrictMock<Agent1_2> agent_1_2;
		fpmas::model::AgentPtr ptr_3 {&agent_1_2};
		NiceMock<MockAgentNode> node_3;

		class MockBuildNode {
			private:
				MockAgentGraph<>::NodeType* node;
				fpmas::model::detail::InsertAgentNodeCallback& insert;
				fpmas::model::detail::SetAgentLocalCallback& set_local;
			public:
				MockBuildNode(
						MockAgentGraph<>::NodeType* node,
						fpmas::model::detail::InsertAgentNodeCallback& insert,
						fpmas::model::detail::SetAgentLocalCallback& set_local)
					: node(node), insert(insert), set_local(set_local) {}

				MockAgentGraph<>::NodeType* build(fpmas::api::model::AgentPtr& ptr) {
					ptr.release();
					insert.call(node);
					set_local.call(node);
					return node;
				}
		};

		MockBuildNode mock_1 {&node_1, insert_callback, set_local_callback};
		MockBuildNode mock_2 {&node_2, insert_callback, set_local_callback};
		MockBuildNode mock_3 {&node_3, insert_callback, set_local_callback};

		void SetUp() override {
			ON_CALL(model, getGroup(0))
				.WillByDefault(ReturnRef(group_1));
			ON_CALL(model, getGroup(1))
				.WillByDefault(ReturnRef(group_2));

			ON_CALL(graph, buildNode_rv(Property(&fpmas::model::AgentPtr::get, &agent_1)))
				.WillByDefault(Invoke(&mock_1, &MockBuildNode::build));
			ON_CALL(graph, buildNode_rv(Property(&fpmas::model::AgentPtr::get, &agent_2)))
				.WillByDefault(Invoke(&mock_2, &MockBuildNode::build));
			ON_CALL(graph, buildNode_rv(Property(&fpmas::model::AgentPtr::get, &agent_1_2)))
				.WillByDefault(Invoke(&mock_3, &MockBuildNode::build));

			ON_CALL(node_1, data())
				.WillByDefault(ReturnRef(ptr_1));
			ON_CALL(node_2, data())
				.WillByDefault(ReturnRef(ptr_2));
			ON_CALL(node_3, data())
				.WillByDefault(ReturnRef(ptr_3));
		}

		void TearDown() override {
			erase_node_callback.call(&node_1);
			erase_node_callback.call(&node_2);
			erase_node_callback.call(&node_3);

			ptr_1.release();
			ptr_2.release();
			ptr_3.release();
		}
};

TEST_F(AgentGroupBehaviorTest, add_simple_behavior) {
	group_1.add(&agent_1);
	group_2.add(&agent_2);

	EXPECT_CALL(agent_1, action_1);
	for(auto task : group_1.job().tasks())
		task->run();

	EXPECT_CALL(agent_2, action_2);
	for(auto task : group_2.job().tasks())
		task->run();
}

TEST_F(AgentGroupBehaviorTest, add_complex_behavior) {
	group_1.add(&agent_1);
	group_1.add(&agent_1_2);
	group_2.add(&agent_2);
	group_2.add(&agent_1_2);
	
	EXPECT_CALL(agent_1, action_1);
	EXPECT_CALL(agent_1_2, action_1);
	for(auto task : group_1.job().tasks())
		task->run();

	EXPECT_CALL(agent_2, action_2);
	EXPECT_CALL(agent_1_2, action_2);
	for(auto task : group_2.job().tasks())
		task->run();
}

/*
 * Ensures that tasks are properly associated to agents when they are
 * "imported" in the graph.
 */
TEST_F(AgentGroupBehaviorTest, insert_callbacks_multi_group) {
	agent_1_2.addGroupId(0);
	agent_1_2.addGroupId(1);

	// Imitates the graph.insert() behavior
	insert_callback.call(&node_3);
	set_local_callback.call(&node_3);
	
	EXPECT_CALL(agent_1_2, action_1);
	for(auto task : group_1.job().tasks())
		task->run();

	EXPECT_CALL(agent_1_2, action_2);
	for(auto task : group_2.job().tasks())
		task->run();
}
