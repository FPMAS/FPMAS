#include "model/model.h"

#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/model/mock_model.h"
#include "../mocks/communication/mock_communication.h"

using ::testing::A;
using ::testing::An;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::IsNull;
using ::testing::Not;
using ::testing::Pair;
using ::testing::Property;
using ::testing::Ref;
using ::testing::ReturnRefOfCopy;
using ::testing::SizeIs;
using ::testing::TypedEq;
using ::testing::UnorderedElementsAre;
using ::testing::WhenDynamicCastTo;

using FPMAS::model::Model;
using FPMAS::model::AgentGroup;
using FPMAS::model::AgentTask;

typedef FPMAS::api::model::AgentPtr AgentPtr;

TEST(AgentTaskTest, build) {
	MockAgent<> agent;
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
	MockAgent<> agent;
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

		typedef
		FPMAS::api::utils::Callback<
			FPMAS::api::graph::parallel::DistributedNode<AgentPtr>*>
			NodeCallback;
		NodeCallback* insert_node_callback;
		NodeCallback* erase_node_callback;
		NodeCallback* set_local_callback;
		NodeCallback* set_distant_callback;

		Model* model;
		void SetUp() override {
			EXPECT_CALL(graph, addCallOnInsertNode(
					WhenDynamicCastTo<FPMAS::model::InsertNodeCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&insert_node_callback));
			EXPECT_CALL(graph, addCallOnEraseNode(
					WhenDynamicCastTo<FPMAS::model::EraseNodeCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&erase_node_callback));
			EXPECT_CALL(graph, addCallOnSetLocal(
					WhenDynamicCastTo<FPMAS::model::SetLocalNodeCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&set_local_callback));
			EXPECT_CALL(graph, addCallOnSetDistant(
					WhenDynamicCastTo<FPMAS::model::SetDistantNodeCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&set_distant_callback));
			model = new Model(graph, scheduler, runtime, load_balancing);
		}

		void TearDown() override {
			EXPECT_CALL(graph, clear);
			delete model;
			// Would normally be deleted by ~Graph()
			delete insert_node_callback;
			delete erase_node_callback;
			delete set_local_callback;
			delete set_distant_callback;
		}
};

TEST_F(ModelTest, graph) {
	ASSERT_THAT(model->graph(), Ref(graph));
}

TEST_F(ModelTest, build_group) {
	auto& group_1 = model->buildGroup();
	auto& group_2 = model->buildGroup();

	ASSERT_THAT(model->groups(), UnorderedElementsAre(Pair(_, &group_1), Pair(_, &group_2)));
}

TEST_F(ModelTest, load_balancing_job) {
	scheduler.schedule(0, model->loadBalancingJob());
	
	MockMpiCommunicator<4, 10> mock_comm;
	EXPECT_CALL(graph, getMpiCommunicator()).Times(AnyNumber())
		.WillRepeatedly(ReturnRef(mock_comm));
	EXPECT_CALL(graph, getNodes).Times(AnyNumber());
	EXPECT_CALL(graph, getArcs).Times(AnyNumber());

	EXPECT_CALL(graph, balance(Ref(load_balancing)));
	runtime.run(1);
}

class AgentGroupTest : public ::testing::Test {
	public:
		typedef FPMAS::api::graph::parallel::DistributedNode<AgentPtr> Node;
	protected:
		MockModel model;
		FPMAS::model::InsertNodeCallback insert_node_callback {model};
		FPMAS::model::EraseNodeCallback erase_node_callback {model};
		FPMAS::model::SetLocalNodeCallback set_local_callback {model};

		FPMAS::api::model::GroupId id = 1;

		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
			graph;
		MockMpiCommunicator<4, 10> mock_comm;
		MockDistributedNode<AgentPtr> node1 {{0, 1}};
		MockDistributedNode<AgentPtr> node2 {{0, 2}};

		AgentGroup agent_group {10, graph, FPMAS::JID(18)};
		MockAgent<>* agent1 = new MockAgent<>;
		FPMAS::api::model::AgentTask* agent1_task;
		AgentPtr agent1_ptr {agent1};

		MockAgent<>* agent2 = new MockAgent<>;
		FPMAS::api::model::AgentTask* agent2_task;
		AgentPtr agent2_ptr {agent2};

		FPMAS::scheduler::Scheduler scheduler;
		FPMAS::runtime::Runtime runtime {scheduler};
/*
 *
 *        class ReturnMockData {
 *            private:
 *                AgentPtr agent;
 *            public:
 *                ReturnMockData(AgentPtr agent) : agent(agent) {}
 *                AgentPtr& operator()() {return agent;}
 *        };
 */

		void SetUp() {
			// In case of LOG
			EXPECT_CALL(model, graph).Times(AnyNumber())
				.WillRepeatedly(ReturnRef(graph));
			EXPECT_CALL(graph, getMpiCommunicator).Times(AnyNumber())
				.WillRepeatedly(ReturnRef(mock_comm));

			ON_CALL(node1, data()).WillByDefault(ReturnRef(agent1_ptr));
			EXPECT_CALL(node1, data()).Times(AnyNumber());
			ON_CALL(node2, data()).WillByDefault(ReturnRef(agent2_ptr));
			EXPECT_CALL(node2, data()).Times(AnyNumber());

			EXPECT_CALL(*agent1, groupId)
				.Times(AnyNumber())
				.WillRepeatedly(Return(id));
			EXPECT_CALL(*agent2, groupId)
				.Times(AnyNumber())
				.WillRepeatedly(Return(id));
			ON_CALL(*agent1, setTask).WillByDefault(SaveArg<0>(&agent1_task));
			EXPECT_CALL(*agent1, task())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&agent1_task));
			ON_CALL(*agent2, setTask).WillByDefault(SaveArg<0>(&agent2_task));
			EXPECT_CALL(*agent2, task())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&agent2_task));

			EXPECT_CALL(graph, insert(A<Node*>()))
				.Times(AnyNumber())
				.WillRepeatedly(DoAll(
							Invoke(&insert_node_callback, &FPMAS::model::InsertNodeCallback::call),
							Invoke(&set_local_callback, &FPMAS::model::SetLocalNodeCallback::call)
							));
			EXPECT_CALL(graph, erase(A<Node*>()))
				.Times(AnyNumber())
				.WillRepeatedly(DoAll(
							Invoke(&erase_node_callback, &FPMAS::model::EraseNodeCallback::call)
							));
			EXPECT_CALL(model, getGroup(id))
				.Times(AnyNumber())
				.WillRepeatedly(ReturnRef(agent_group));
		}
};

TEST_F(AgentGroupTest, id) {
	ASSERT_EQ(agent_group.groupId(), 10);
	//delete agent1;
	//delete agent2;
}

TEST_F(AgentGroupTest, job) {
	ASSERT_EQ(agent_group.job().id(), FPMAS::JID(18));
	//delete agent1;
	//delete agent2;
}

TEST_F(AgentGroupTest, job_end) {
	EXPECT_CALL(graph, synchronize);

	agent_group.job().getEndTask().run();
	//delete agent1;
	//delete agent2;
}

class MockBuildNode {
	private:
		typedef FPMAS::api::graph::parallel::DistributedGraph<AgentPtr>
			Graph;
		Graph* graph;
		typename AgentGroupTest::Node* node;

	public:
		MockBuildNode(Graph* graph, typename AgentGroupTest::Node* node)
			: graph(graph), node(node) {}

		void operator()() {
			graph->insert(node);

		}
};

TEST_F(AgentGroupTest, add_agent) {
	EXPECT_CALL(*agent1, setTask);
	EXPECT_CALL(*agent2, setTask);

	// Agent 1 set up
	MockBuildNode build_node_1 {&graph, &node1};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_1),
			Return(&node1)));
	EXPECT_CALL(*agent1, setNode(&node1));

	// The fake agent will be implicitely and automatically deleted from the
	// temporary AgentPtr in the mocked buildNode function. In consequence, we
	// don't use the real "agent1", BUT agent 1 is still returned by the
	// buildNode function (see expectation above)
	MockAgent<>* fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	// Agent 2 set up
	MockBuildNode build_node_2 {&graph, &node2};
	//EXPECT_CALL(graph, buildNode_rv(Pointee(Property(&AgentPtr::get, agent2))))
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_2),
			Return(&node1)));
	EXPECT_CALL(*agent2, setNode(&node2));

	fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	// Would normally be called from the Graph destructor
	erase_node_callback.call(&node1);
	erase_node_callback.call(&node2);
}

TEST_F(AgentGroupTest, agent_task) {
	EXPECT_CALL(*agent1, setTask);
	EXPECT_CALL(*agent2, setTask);

	MockBuildNode build_node_1 {&graph, &node1};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_1),
			Return(&node1)));
	EXPECT_CALL(*agent1, setNode);

	MockAgent<>* fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	MockBuildNode build_node_2 {&graph, &node2};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_2),
			Return(&node2)));
	EXPECT_CALL(*agent2, setNode);

	fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	ASSERT_THAT(agent_group.job().tasks(), SizeIs(2));

	EXPECT_CALL(graph, synchronize);
	::testing::Sequence s1, s2;
	EXPECT_CALL(*agent1, act)
		.InSequence(s1);
	EXPECT_CALL(*agent2, act)
		.InSequence(s2);

	scheduler.schedule(0, agent_group.job());
	runtime.run(1);

	// Would normally be called from the Graph destructor
	erase_node_callback.call(&node1);
	erase_node_callback.call(&node2);
}
