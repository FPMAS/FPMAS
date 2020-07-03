#include "fpmas/model/model.h"

#include "../mocks/graph/parallel/mock_distributed_arc.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/graph/parallel/mock_distributed_graph.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/model/mock_model.h"
#include "../mocks/communication/mock_communication.h"
#include "../mocks/synchro/mock_mutex.h"
#include "fpmas/model/guards.h"

using ::testing::A;
using ::testing::An;
using ::testing::Assign;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::IsEmpty;
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

using fpmas::model::Model;
using fpmas::model::AgentGroup;
using fpmas::model::AgentTask;

typedef fpmas::api::model::AgentPtr AgentPtr;

TEST(AgentTaskTest, build) {
	MockAgent<> agent;
	MockDistributedNode<AgentPtr> node;

	AgentTask agent_task;
	agent_task.setAgent(&agent);
	ON_CALL(agent, node())
		.WillByDefault(Return(&node));
	EXPECT_CALL(agent, node()).Times(AnyNumber());
	ON_CALL(Const(agent), node())
		.WillByDefault(Return(&node));
	EXPECT_CALL(Const(agent), node()).Times(AnyNumber());

	ASSERT_THAT(agent_task.agent(), &agent);
	ASSERT_THAT(agent_task.node(), Eq(&node));
}

TEST(AgentTaskTest, run) {
	MockAgent<> agent;
	AgentTask agent_task;
	agent_task.setAgent(&agent);

	EXPECT_CALL(agent, act);

	agent_task.run();
}

class ModelTest : public ::testing::Test {
	public:
		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
			graph;
		MockLoadBalancing<AgentPtr> load_balancing;

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};

		typedef
		fpmas::api::utils::Callback<
			fpmas::api::graph::parallel::DistributedNode<AgentPtr>*>
			NodeCallback;
		NodeCallback* insert_node_callback;
		NodeCallback* erase_node_callback;
		NodeCallback* set_local_callback;
		NodeCallback* set_distant_callback;

		Model* model;
		void SetUp() override {
			EXPECT_CALL(graph, addCallOnInsertNode(
					WhenDynamicCastTo<fpmas::model::InsertAgentCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&insert_node_callback));
			EXPECT_CALL(graph, addCallOnEraseNode(
					WhenDynamicCastTo<fpmas::model::EraseAgentCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&erase_node_callback));
			EXPECT_CALL(graph, addCallOnSetLocal(
					WhenDynamicCastTo<fpmas::model::SetAgentLocalCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&set_local_callback));
			EXPECT_CALL(graph, addCallOnSetDistant(
					WhenDynamicCastTo<fpmas::model::SetAgentDistantCallback*>(Not(IsNull()))
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
		typedef fpmas::api::graph::parallel::DistributedNode<AgentPtr> Node;
	protected:
		MockModel model;
		fpmas::model::InsertAgentCallback insert_agent_callback {model};
		fpmas::model::EraseAgentCallback erase_agent_callback {model};
		fpmas::model::SetAgentLocalCallback set_local_callback {model};

		fpmas::api::model::GroupId id = 1;

		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
			graph;
		MockMpiCommunicator<4, 10> mock_comm;
		MockDistributedNode<AgentPtr> node1 {{0, 1}};
		MockDistributedNode<AgentPtr> node2 {{0, 2}};

		AgentGroup agent_group {10, graph, fpmas::JID(18)};
		MockAgent<>* agent1 = new MockAgent<>;
		fpmas::api::model::AgentTask* agent1_task;
		AgentPtr agent1_ptr {agent1};

		MockAgent<>* agent2 = new MockAgent<>;
		fpmas::api::model::AgentTask* agent2_task;
		AgentPtr agent2_ptr {agent2};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};

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
							Invoke(&insert_agent_callback, &fpmas::model::InsertAgentCallback::call),
							Invoke(&set_local_callback, &fpmas::model::SetAgentLocalCallback::call)
							));
			EXPECT_CALL(graph, erase(A<Node*>()))
				.Times(AnyNumber())
				.WillRepeatedly(DoAll(
							Invoke(&erase_agent_callback, &fpmas::model::EraseAgentCallback::call)
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
	ASSERT_EQ(agent_group.job().id(), fpmas::JID(18));
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
		typedef fpmas::api::graph::parallel::DistributedGraph<AgentPtr>
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
	EXPECT_CALL(*agent1, setGraph(&graph));

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
	EXPECT_CALL(*agent2, setGraph(&graph));

	fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	// Would normally be called from the Graph destructor
	erase_agent_callback.call(&node1);
	erase_agent_callback.call(&node2);
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
	EXPECT_CALL(*agent1, setGraph);

	MockAgent<>* fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	MockBuildNode build_node_2 {&graph, &node2};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_2),
			Return(&node2)));
	EXPECT_CALL(*agent2, setNode);
	EXPECT_CALL(*agent2, setGraph);

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
	erase_agent_callback.call(&node1);
	erase_agent_callback.call(&node2);
}

class AgentBaseTest : public ::testing::Test {
	protected:
		MockAgentBase<10> agent_base;
};

TEST_F(AgentBaseTest, type_id) {
	ASSERT_EQ(agent_base.typeId(), 10);
}

TEST_F(AgentBaseTest, group_id) {
	agent_base.setGroupId(12);
	ASSERT_EQ(agent_base.groupId(), 12);
}

TEST_F(AgentBaseTest, node) {
	MockDistributedNode<AgentPtr> node;
	agent_base.setNode(&node);
	ASSERT_EQ(agent_base.node(), &node);
}

TEST_F(AgentBaseTest, graph) {
	MockDistributedGraph<
		AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedArc<AgentPtr>>
		graph;
	agent_base.setGraph(&graph);
	ASSERT_EQ(agent_base.graph(), &graph);
}

TEST_F(AgentBaseTest, task) {
	AgentTask task;
	agent_base.setTask(&task);
	ASSERT_EQ(agent_base.task(), &task);
}

TEST_F(AgentBaseTest, out_neighbors) {
	MockDistributedNode<AgentPtr> n_1 {{0, 0}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_1;
	EXPECT_CALL(a_1, getTargetNode).Times(AnyNumber()).WillRepeatedly(Return(&n_1));

	MockDistributedNode<AgentPtr> n_2 {{0, 1}, new MockAgentBase<12>};
	MockDistributedArc<AgentPtr> a_2;
	EXPECT_CALL(a_2, getTargetNode).Times(AnyNumber()).WillRepeatedly(Return(&n_2));

	MockDistributedNode<AgentPtr> n_3 {{0, 2}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_3;
	EXPECT_CALL(a_3, getTargetNode).Times(AnyNumber()).WillRepeatedly(Return(&n_3));

	MockAgentBase<10>* agent = new MockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::parallel::DistributedArc<AgentPtr>*> out_arcs {&a_1, &a_2, &a_3};
	EXPECT_CALL(n, getOutgoingArcs()).Times(AnyNumber()).WillRepeatedly(
			Return(out_arcs)
			);

	agent->setNode(&n);

	std::vector<MockAgentBase<8>*> out_8 = agent->outNeighbors<MockAgentBase<8>>();
	ASSERT_THAT(out_8, UnorderedElementsAre(n_1.data().get(), n_3.data().get()));

	std::vector<MockAgentBase<12>*> out_12 = agent->outNeighbors<MockAgentBase<12>>();
	ASSERT_THAT(out_12, ElementsAre(n_2.data().get()));

	std::vector<MockAgentBase<84>*> other_out = agent->outNeighbors<MockAgentBase<84>>();
	ASSERT_THAT(other_out, IsEmpty());
}

TEST_F(AgentBaseTest, shuffle_out_neighbors) {
	MockDistributedNode<AgentPtr> n_1 {{0, 0}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_1;
	EXPECT_CALL(a_1, getTargetNode).Times(AnyNumber()).WillRepeatedly(Return(&n_1));

	MockDistributedNode<AgentPtr> n_2 {{0, 1}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_2;
	EXPECT_CALL(a_2, getTargetNode).Times(AnyNumber()).WillRepeatedly(Return(&n_2));

	MockDistributedNode<AgentPtr> n_3 {{0, 2}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_3;
	EXPECT_CALL(a_3, getTargetNode).Times(AnyNumber()).WillRepeatedly(Return(&n_3));

	MockAgentBase<10>* agent = new MockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::parallel::DistributedArc<AgentPtr>*> out_arcs {&a_1, &a_2, &a_3};
	EXPECT_CALL(n, getOutgoingArcs()).Times(AnyNumber()).WillRepeatedly(
			Return(out_arcs)
			);

	agent->setNode(&n);

	fpmas::model::Neighbors<MockAgentBase<8>> out = agent->outNeighbors<MockAgentBase<8>>();
	std::vector<MockAgentBase<8>*> out_1 = out.shuffle();
	std::vector<MockAgentBase<8>*> out_2 = out.shuffle();
	std::vector<MockAgentBase<8>*> out_3 = out.shuffle();

	ASSERT_THAT(out_2, Not(ElementsAreArray(out_1)));
	ASSERT_THAT(out_3, Not(ElementsAreArray(out_1)));
}

TEST_F(AgentBaseTest, in_neighbors) {
	MockDistributedNode<AgentPtr> n_1 {{0, 0}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_1;
	EXPECT_CALL(a_1, getSourceNode).Times(AnyNumber()).WillRepeatedly(Return(&n_1));

	MockDistributedNode<AgentPtr> n_2 {{0, 1}, new MockAgentBase<12>};
	MockDistributedArc<AgentPtr> a_2;
	EXPECT_CALL(a_2, getSourceNode).Times(AnyNumber()).WillRepeatedly(Return(&n_2));

	MockDistributedNode<AgentPtr> n_3 {{0, 2}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_3;
	EXPECT_CALL(a_3, getSourceNode).Times(AnyNumber()).WillRepeatedly(Return(&n_3));

	MockAgentBase<10>* agent = new MockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::parallel::DistributedArc<AgentPtr>*> in_arcs {&a_1, &a_2, &a_3};
	EXPECT_CALL(n, getIncomingArcs()).Times(AnyNumber()).WillRepeatedly(
			Return(in_arcs)
			);

	agent->setNode(&n);

	std::vector<MockAgentBase<8>*> in_8 = agent->inNeighbors<MockAgentBase<8>>();
	ASSERT_THAT(in_8, UnorderedElementsAre(n_1.data().get(), n_3.data().get()));

	std::vector<MockAgentBase<12>*> in_12 = agent->inNeighbors<MockAgentBase<12>>();
	ASSERT_THAT(in_12, ElementsAre(n_2.data().get()));

	std::vector<MockAgentBase<84>*> other_in = agent->inNeighbors<MockAgentBase<84>>();
	ASSERT_THAT(other_in, IsEmpty());
}

TEST_F(AgentBaseTest, shuffle_in_neighbors) {
	MockDistributedNode<AgentPtr> n_1 {{0, 0}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_1;
	EXPECT_CALL(a_1, getSourceNode).Times(AnyNumber()).WillRepeatedly(Return(&n_1));

	MockDistributedNode<AgentPtr> n_2 {{0, 1}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_2;
	EXPECT_CALL(a_2, getSourceNode).Times(AnyNumber()).WillRepeatedly(Return(&n_2));

	MockDistributedNode<AgentPtr> n_3 {{0, 2}, new MockAgentBase<8>};
	MockDistributedArc<AgentPtr> a_3;
	EXPECT_CALL(a_3, getSourceNode).Times(AnyNumber()).WillRepeatedly(Return(&n_3));

	MockAgentBase<10>* agent = new MockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::parallel::DistributedArc<AgentPtr>*> in_arcs {&a_1, &a_2, &a_3};
	EXPECT_CALL(n, getIncomingArcs()).Times(AnyNumber()).WillRepeatedly(
			Return(in_arcs)
			);

	agent->setNode(&n);

	fpmas::model::Neighbors<MockAgentBase<8>> in = agent->inNeighbors<MockAgentBase<8>>();
	std::vector<MockAgentBase<8>*> in_1 = in.shuffle();
	std::vector<MockAgentBase<8>*> in_2 = in.shuffle();
	std::vector<MockAgentBase<8>*> in_3 = in.shuffle();

	ASSERT_THAT(in_2, Not(ElementsAreArray(in_1)));
	ASSERT_THAT(in_3, Not(ElementsAreArray(in_1)));
}
class FakeAgent : public MockAgentBase<10> {
	public:
		int field = 0;
};

class AgentGuardTest : public ::testing::Test {
	protected:
		FakeAgent* agent = new FakeAgent;
		MockDistributedNode<AgentPtr> node {{0, 0}, agent};
		MockMutex<AgentPtr> mutex;

		void SetUp() {
			agent->setNode(&node);
			EXPECT_CALL(node, mutex()).Times(AnyNumber())
				.WillRepeatedly(ReturnRef(mutex));
		}
};

TEST_F(AgentGuardTest, read_guard) {
	FakeAgent* agent_ptr = static_cast<FakeAgent*>(node.data().get());

	EXPECT_CALL(mutex, read).WillOnce(DoAll(Assign(&agent->field, 14), ReturnRef(node.data())));
	const fpmas::model::ReadGuard read(agent_ptr);

	// Pointed data has implicitely been updated by mutex::read
	ASSERT_EQ(agent_ptr->field, 14);

	EXPECT_CALL(mutex, releaseRead);
}

TEST_F(AgentGuardTest, acquire_guard) {
	FakeAgent* agent_ptr = static_cast<FakeAgent*>(node.data().get());

	EXPECT_CALL(mutex, acquire).WillOnce(DoAll(Assign(&agent->field, 14), ReturnRef(node.data())));
	const fpmas::model::AcquireGuard acq(agent_ptr);

	// Pointed data has implicitely been updated by mutex::acquire
	ASSERT_EQ(agent_ptr->field, 14);

	EXPECT_CALL(mutex, releaseAcquire);
}
