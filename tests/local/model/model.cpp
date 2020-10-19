#include "fpmas/model/model.h"

#include "../mocks/graph/mock_distributed_edge.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/graph/mock_distributed_graph.h"
#include "../mocks/graph/mock_load_balancing.h"
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
using ::testing::Pointee;
using ::testing::Property;
using ::testing::Ref;
using ::testing::ReturnRefOfCopy;
using ::testing::SizeIs;
using ::testing::TypedEq;
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;
using ::testing::WhenDynamicCastTo;
using ::testing::NiceMock;

using fpmas::model::detail::Model;
using fpmas::model::detail::AgentGroup;
using fpmas::model::detail::AgentTask;

typedef fpmas::api::model::AgentPtr AgentPtr;

class AgentTaskTest : public ::testing::Test {
	protected:
		MockAgent<> agent;
		AgentPtr agent_ptr {&agent};
		MockDistributedNode<AgentPtr> node;

		void SetUp() override {
			ON_CALL(agent, node())
				.WillByDefault(Return(&node));
			EXPECT_CALL(agent, node()).Times(AnyNumber());
			ON_CALL(Const(agent), node())
				.WillByDefault(Return(&node));
			EXPECT_CALL(Const(agent), node()).Times(AnyNumber());
		}

		void TearDown() override {
			agent_ptr.release();
		}

};

TEST_F(AgentTaskTest, build) {
	AgentTask agent_task(agent_ptr);
	//agent_task.setAgent(&agent);

	ASSERT_THAT(agent_task.agent(), Ref(agent_ptr));
	ASSERT_THAT(agent_task.node(), Eq(&node));
}

TEST_F(AgentTaskTest, run) {
	AgentTask agent_task(agent_ptr);
	EXPECT_CALL(agent, act);

	agent_task.run();
}

class ModelTest : public ::testing::Test {
	public:
		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedEdge<AgentPtr>>
			graph;
		MockLoadBalancing<AgentPtr> load_balancing;

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};

		typedef
		fpmas::api::utils::Callback<
			fpmas::api::graph::DistributedNode<AgentPtr>*>
			NodeCallback;
		NodeCallback* insert_node_callback;
		NodeCallback* erase_node_callback;
		NodeCallback* set_local_callback;
		NodeCallback* set_distant_callback;

		Model* model;
		void SetUp() override {
			EXPECT_CALL(graph, addCallOnInsertNode(
					WhenDynamicCastTo<fpmas::model::detail::InsertAgentNodeCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&insert_node_callback));
			EXPECT_CALL(graph, addCallOnEraseNode(
					WhenDynamicCastTo<fpmas::model::detail::EraseAgentNodeCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&erase_node_callback));
			EXPECT_CALL(graph, addCallOnSetLocal(
					WhenDynamicCastTo<fpmas::model::detail::SetAgentLocalCallback*>(Not(IsNull()))
					)).WillOnce(SaveArg<0>(&set_local_callback));
			EXPECT_CALL(graph, addCallOnSetDistant(
					WhenDynamicCastTo<fpmas::model::detail::SetAgentDistantCallback*>(Not(IsNull()))
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
	FPMAS_DEFINE_GROUPS(G_1, G_2);

	auto& group_1 = model->buildGroup(G_1);
	auto& group_2 = model->buildGroup(G_2);

	ASSERT_THAT(model->groups(), UnorderedElementsAre(Pair(G_1, &group_1), Pair(G_2, &group_2)));
}

TEST_F(ModelTest, load_balancing_job) {
	scheduler.schedule(0, model->loadBalancingJob());
	
	MockMpiCommunicator<4, 10> mock_comm;
	EXPECT_CALL(graph, getMpiCommunicator()).Times(AnyNumber())
		.WillRepeatedly(ReturnRef(mock_comm));
	EXPECT_CALL(graph, getNodes).Times(AnyNumber());
	EXPECT_CALL(graph, getEdges).Times(AnyNumber());

	EXPECT_CALL(graph, balance(Ref(load_balancing)));
	runtime.run(1);
}

TEST_F(ModelTest, link) {
	NiceMock<MockAgent<0>> agent_1;
	MockDistributedNode<AgentPtr> node_1;
	ON_CALL(agent_1, node()).WillByDefault(Return(&node_1));

	MockDistributedNode<AgentPtr> node_2;
	NiceMock<MockAgent<4>> agent_2;
	ON_CALL(agent_2, node()).WillByDefault(Return(&node_2));

	fpmas::api::graph::LayerId layer = 12;

	EXPECT_CALL(graph, link(&node_1, &node_2, layer));
	model->link(&agent_1, &agent_2, layer);
}

TEST_F(ModelTest, unlink) {
	MockDistributedEdge<AgentPtr> mock_edge;

	EXPECT_CALL(graph, unlink(&mock_edge));
	model->unlink(&mock_edge);
}

class AgentGroupTest : public ::testing::Test {
	public:
		typedef fpmas::api::graph::DistributedNode<AgentPtr> Node;
	protected:
		MockModel model;
		fpmas::model::detail::InsertAgentNodeCallback insert_agent_callback {model};
		fpmas::model::detail::EraseAgentNodeCallback erase_agent_callback {model};
		fpmas::model::detail::SetAgentLocalCallback set_local_callback {model};

		fpmas::api::model::GroupId id = 1;

		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedEdge<AgentPtr>>
			graph;
		MockMpiCommunicator<4, 10> mock_comm;
		MockDistributedNode<AgentPtr> node1 {{0, 1}};
		MockDistributedNode<AgentPtr> node2 {{0, 2}};

		AgentGroup agent_group {10, graph};
		MockAgent<> agent1;
		fpmas::api::model::AgentGroup* agent1_group;
		fpmas::api::model::AgentTask* agent1_task;
		AgentPtr agent1_ptr {&agent1};

		MockAgent<> agent2;
		fpmas::api::model::AgentGroup* agent2_group;
		fpmas::api::model::AgentTask* agent2_task;
		AgentPtr agent2_ptr {&agent2};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};

		void SetUp() override {
			// In case of LOG
			EXPECT_CALL(model, graph).Times(AnyNumber())
				.WillRepeatedly(ReturnRef(graph));
			EXPECT_CALL(graph, getMpiCommunicator).Times(AnyNumber())
				.WillRepeatedly(ReturnRef(mock_comm));

			ON_CALL(node1, data()).WillByDefault(ReturnRef(agent1_ptr));
			EXPECT_CALL(node1, data()).Times(AnyNumber());
			ON_CALL(node2, data()).WillByDefault(ReturnRef(agent2_ptr));
			EXPECT_CALL(node2, data()).Times(AnyNumber());

			EXPECT_CALL(agent1, groupId)
				.Times(AnyNumber())
				.WillRepeatedly(Return(id));
			EXPECT_CALL(agent2, groupId)
				.Times(AnyNumber())
				.WillRepeatedly(Return(id));

			// Task SetUp
			ON_CALL(agent1, setTask).WillByDefault(SaveArg<0>(&agent1_task));
			EXPECT_CALL(agent1, task())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&agent1_task));
			ON_CALL(agent2, setTask).WillByDefault(SaveArg<0>(&agent2_task));
			EXPECT_CALL(agent2, task())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&agent2_task));

			// AgentGroup SetUp
			ON_CALL(agent1, setGroup).WillByDefault(SaveArg<0>(&agent1_group));
			EXPECT_CALL(agent1, group())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&agent1_group));
			ON_CALL(agent2, setGroup).WillByDefault(SaveArg<0>(&agent2_group));
			EXPECT_CALL(agent2, group())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(&agent2_group));

			EXPECT_CALL(graph, insert(A<Node*>()))
				.Times(AnyNumber())
				.WillRepeatedly(DoAll(
							Invoke(&insert_agent_callback, &fpmas::model::detail::InsertAgentNodeCallback::call),
							Invoke(&set_local_callback, &fpmas::model::detail::SetAgentLocalCallback::call)
							));
			EXPECT_CALL(graph, erase(A<Node*>()))
				.Times(AnyNumber())
				.WillRepeatedly(DoAll(
							Invoke(&erase_agent_callback, &fpmas::model::detail::EraseAgentNodeCallback::call)
							));
			EXPECT_CALL(model, getGroup(id))
				.Times(AnyNumber())
				.WillRepeatedly(ReturnRef(agent_group));
		}

		void TearDown() override {
			agent1_ptr.release();
			agent2_ptr.release();
		}
};

TEST_F(AgentGroupTest, id) {
	ASSERT_EQ(agent_group.groupId(), 10);
}

TEST_F(AgentGroupTest, job_end) {
	EXPECT_CALL(graph, synchronize);

	agent_group.job().getEndTask().run();
}

class MockBuildNode {
	private:
		typedef fpmas::api::graph::DistributedGraph<AgentPtr>
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

	EXPECT_CALL(agent1, setTask);
	// Agent 1 set up
	MockBuildNode build_node_1 {&graph, &node1};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_1),
			Return(&node1)));
	EXPECT_CALL(agent1, setNode(&node1));
	EXPECT_CALL(agent1, setModel(&model));
	EXPECT_CALL(agent1, setGroup(&agent_group));

	std::array<MockAgent<>*, 2> fake_agents {new MockAgent<>, new MockAgent<>};
	// The fake agent will be implicitely and automatically deleted from the
	// temporary AgentPtr in the mocked buildNode function. In consequence, we
	// don't use the real "agent1", BUT agent 1 is still returned by the
	// buildNode function (see expectation above)
	EXPECT_CALL(*fake_agents[0], setGroupId(10));
	agent_group.add(fake_agents[0]);

	EXPECT_CALL(agent2, setTask);
	// Agent 2 set up
	MockBuildNode build_node_2 {&graph, &node2};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_2),
			Return(&node1)));
	EXPECT_CALL(agent2, setNode(&node2));
	EXPECT_CALL(agent2, setModel(&model));
	EXPECT_CALL(agent2, setGroup(&agent_group));

	EXPECT_CALL(*fake_agents[1], setGroupId(10));
	agent_group.add(fake_agents[1]);

	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(&agent1, &agent2));

	// Would normally be called from the Graph destructor
	erase_agent_callback.call(&node1);
	erase_agent_callback.call(&node2);
}

TEST_F(AgentGroupTest, insert_agent) {
	agent_group.insert(&agent1_ptr);
	agent_group.insert(&agent2_ptr);

	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(&agent1, &agent2));
}

TEST_F(AgentGroupTest, erase_agent) {
	agent_group.insert(&agent1_ptr);
	agent_group.insert(&agent2_ptr);

	agent_group.erase(&agent1_ptr);
	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(&agent2));
	agent_group.erase(&agent2_ptr);
	ASSERT_THAT(agent_group.agents(), IsEmpty());
}

TEST_F(AgentGroupTest, agent_task) {
	EXPECT_CALL(agent1, setTask);
	EXPECT_CALL(agent2, setTask);

	MockBuildNode build_node_1 {&graph, &node1};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_1),
			Return(&node1)));
	EXPECT_CALL(agent1, setNode);
	EXPECT_CALL(agent1, setModel);
	EXPECT_CALL(agent1, setGroup);

	MockAgent<>* fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	MockBuildNode build_node_2 {&graph, &node2};
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(DoAll(
			InvokeWithoutArgs(build_node_2),
			Return(&node2)));
	EXPECT_CALL(agent2, setNode);
	EXPECT_CALL(agent2, setModel);
	EXPECT_CALL(agent2, setGroup);

	fake_agent = new MockAgent<>;
	EXPECT_CALL(*fake_agent, setGroupId(10));
	agent_group.add(fake_agent);

	ASSERT_THAT(agent_group.job().tasks(), SizeIs(2));

	EXPECT_CALL(graph, synchronize);
	::testing::Sequence s1, s2;

	EXPECT_CALL(agent1, act)
		.InSequence(s1);

	EXPECT_CALL(agent2, act)
		.InSequence(s2);

	scheduler.schedule(0, agent_group.job());
	runtime.run(1);

	// Would normally be called from the Graph destructor
	erase_agent_callback.call(&node1);
	erase_agent_callback.call(&node2);
}

TEST_F(AgentGroupTest, local_agents) {
	agent_group.insert(&agent1_ptr);
	agent_group.insert(&agent2_ptr);

	ON_CALL(agent1, node())
		.WillByDefault(Return(&node1));
	EXPECT_CALL(agent1, node()).Times(AnyNumber());
	ON_CALL(agent2, node())
		.WillByDefault(Return(&node2));
	EXPECT_CALL(agent2, node()).Times(AnyNumber());

	EXPECT_CALL(node1, state)
		.WillRepeatedly(Return(LocationState::LOCAL));
	EXPECT_CALL(node2, state)
		.WillRepeatedly(Return(LocationState::LOCAL));

	ASSERT_THAT(agent_group.localAgents(), UnorderedElementsAre(&agent1, &agent2));

	EXPECT_CALL(node1, state)
		.WillRepeatedly(Return(LocationState::DISTANT));

	ASSERT_THAT(agent_group.localAgents(), ElementsAre(&agent2));

	EXPECT_CALL(node2, state)
		.WillRepeatedly(Return(LocationState::DISTANT));

	ASSERT_THAT(agent_group.localAgents(), IsEmpty());
}

/*
 *TEST_F(AgentGroupTest, typed_agents) {
 *    MockAgent<1>* agent3 = new MockAgent<1>;
 *    AgentPtr agent3_ptr {agent3};
 *    agent_group.insert(&agent1_ptr);
 *    agent_group.insert(&agent2_ptr);
 *    agent_group.insert(&agent3_ptr);
 *
 *    auto list1 = agent_group.agents<MockAgent<>>();
 *    ASSERT_THAT(list1, UnorderedElementsAre(agent1, agent2));
 *
 *    auto list2 = agent_group.agents<MockAgent<1>>();
 *    ASSERT_THAT(list2, UnorderedElementsAre(agent3));
 *}
 */


class AgentBaseTest : public ::testing::Test {
	protected:
		DefaultMockAgentBase<10> agent_base;
		const DefaultMockAgentBase<10>& const_agent_base = agent_base;
};

TEST_F(AgentBaseTest, type_id) {
	ASSERT_EQ(agent_base.typeId(), std::type_index(typeid(DefaultMockAgentBase<10>)));
}

TEST_F(AgentBaseTest, group_id) {
	agent_base.setGroupId(12);
	ASSERT_EQ(agent_base.groupId(), 12);
}

TEST_F(AgentBaseTest, group) {
	MockAgentGroup group;
	agent_base.setGroup(&group);
	ASSERT_EQ(agent_base.group(), &group);
	ASSERT_EQ(const_agent_base.group(), &group);
}

TEST_F(AgentBaseTest, node) {
	MockDistributedNode<AgentPtr> node;
	agent_base.setNode(&node);
	ASSERT_EQ(agent_base.node(), &node);
}

TEST_F(AgentBaseTest, model) {
	MockModel model;
	agent_base.setModel(&model);
	ASSERT_EQ(agent_base.model(), &model);
}

TEST_F(AgentBaseTest, task) {
	AgentPtr ptr(&agent_base);
	AgentTask task(ptr);
	agent_base.setTask(&task);
	ASSERT_EQ(agent_base.task(), &task);

	ptr.release();
}

TEST_F(AgentBaseTest, out_neighbors) {
	MockDistributedNode<AgentPtr> n_1 {{0, 0}, new DefaultMockAgentBase<8>};
	MockDistributedNode<AgentPtr> n_2 {{0, 1}, new DefaultMockAgentBase<12>};
	MockDistributedNode<AgentPtr> n_3 {{0, 2}, new DefaultMockAgentBase<8>};

	DefaultMockAgentBase<10>* agent = new DefaultMockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*> out_neighbors {&n_1, &n_2, &n_3};
	EXPECT_CALL(n, outNeighbors()).Times(AnyNumber()).WillRepeatedly(
			Return(out_neighbors)
			);

	agent->setNode(&n);

	fpmas::model::Neighbors<DefaultMockAgentBase<8>> out_8 = agent->outNeighbors<DefaultMockAgentBase<8>>();
	std::vector<DefaultMockAgentBase<8>*> out_8_v {out_8.begin(), out_8.end()};
	ASSERT_THAT(out_8_v, UnorderedElementsAre(n_1.data().get(), n_3.data().get()));

	fpmas::model::Neighbors<DefaultMockAgentBase<12>> out_12 = agent->outNeighbors<DefaultMockAgentBase<12>>();
	std::vector<DefaultMockAgentBase<12>*> out_12_v {out_12.begin(), out_12.end()};
	ASSERT_THAT(out_12_v, ElementsAre(n_2.data().get()));

	fpmas::model::Neighbors<DefaultMockAgentBase<84>> other_out = agent->outNeighbors<DefaultMockAgentBase<84>>();
	std::vector<DefaultMockAgentBase<84>*> other_out_v {other_out.begin(), other_out.end()};
	ASSERT_THAT(other_out_v, IsEmpty());
}

TEST_F(AgentBaseTest, shuffle_out_neighbors) {
	std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*> out_neighbors;
	for(unsigned int i = 0; i < 1000; i++) {
		out_neighbors.push_back(new MockDistributedNode<AgentPtr>({0, i}, new DefaultMockAgentBase<8>));
	}

	DefaultMockAgentBase<10>* agent = new DefaultMockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	EXPECT_CALL(n, outNeighbors()).Times(AnyNumber()).WillRepeatedly(
			Return(out_neighbors)
			);

	agent->setNode(&n);

	fpmas::model::Neighbors<DefaultMockAgentBase<8>> out = agent->outNeighbors<DefaultMockAgentBase<8>>();
	std::vector<DefaultMockAgentBase<8>*> out_v {out.begin(), out.end()};
	fpmas::model::Neighbors<DefaultMockAgentBase<8>> out_1 = out.shuffle();
	std::vector<DefaultMockAgentBase<8>*> out_1_v {out_1.begin(), out_1.end()};
	fpmas::model::Neighbors<DefaultMockAgentBase<8>> out_2 = out.shuffle();
	std::vector<DefaultMockAgentBase<8>*> out_2_v {out_2.begin(), out_2.end()};
	fpmas::model::Neighbors<DefaultMockAgentBase<8>> out_3 = out.shuffle();
	std::vector<DefaultMockAgentBase<8>*> out_3_v {out_3.begin(), out_3.end()};

	ASSERT_THAT(out_1_v, Not(ElementsAreArray(out_v)));
	ASSERT_THAT(out_2_v, Not(ElementsAreArray(out_v)));
	ASSERT_THAT(out_3_v, Not(ElementsAreArray(out_v)));

	ASSERT_THAT(out_2_v, Not(ElementsAreArray(out_1_v)));
	ASSERT_THAT(out_3_v, Not(ElementsAreArray(out_2_v)));
	ASSERT_THAT(out_3_v, Not(ElementsAreArray(out_1_v)));

	for(auto neighbor : out_neighbors)
		delete neighbor;
}

TEST_F(AgentBaseTest, in_neighbors) {
	MockDistributedNode<AgentPtr> n_1 {{0, 0}, new DefaultMockAgentBase<8>};
	MockDistributedNode<AgentPtr> n_2 {{0, 1}, new DefaultMockAgentBase<12>};
	MockDistributedNode<AgentPtr> n_3 {{0, 2}, new DefaultMockAgentBase<8>};

	DefaultMockAgentBase<10>* agent = new DefaultMockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*> in_neighbors {&n_1, &n_2, &n_3};
	EXPECT_CALL(n, inNeighbors()).Times(AnyNumber()).WillRepeatedly(
			Return(in_neighbors)
			);

	agent->setNode(&n);

	fpmas::model::Neighbors<DefaultMockAgentBase<8>> in_8 = agent->inNeighbors<DefaultMockAgentBase<8>>();
	std::vector<DefaultMockAgentBase<8>*> in_8_v {in_8.begin(), in_8.end()};
	ASSERT_THAT(in_8_v, UnorderedElementsAre(n_1.data().get(), n_3.data().get()));

	fpmas::model::Neighbors<DefaultMockAgentBase<12>> in_12 = agent->inNeighbors<DefaultMockAgentBase<12>>();
	std::vector<DefaultMockAgentBase<12>*> in_12_v {in_12.begin(), in_12.end()};
	ASSERT_THAT(in_12_v, ElementsAre(n_2.data().get()));

	fpmas::model::Neighbors<DefaultMockAgentBase<84>> other_in = agent->inNeighbors<DefaultMockAgentBase<84>>();
	std::vector<DefaultMockAgentBase<84>*> other_in_v {other_in.begin(), other_in.end()};
	ASSERT_THAT(other_in_v, IsEmpty());
}

TEST_F(AgentBaseTest, shuffle_in_neighbors) {
	std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*> in_neighbors;
	for(unsigned int i = 0; i < 1000; i++) {
		in_neighbors.push_back(new MockDistributedNode<AgentPtr>({0, i}, new DefaultMockAgentBase<8>));
	}

	DefaultMockAgentBase<10>* agent = new DefaultMockAgentBase<10>;
	MockDistributedNode<AgentPtr> n {{0, 4}, agent};
	EXPECT_CALL(n, inNeighbors()).Times(AnyNumber()).WillRepeatedly(
			Return(in_neighbors)
			);

	agent->setNode(&n);

	fpmas::model::Neighbors<DefaultMockAgentBase<8>> in = agent->inNeighbors<DefaultMockAgentBase<8>>();
	std::vector<DefaultMockAgentBase<8>*> in_v {in.begin(), in.end()};
	fpmas::model::Neighbors<DefaultMockAgentBase<8>> in_1 = in.shuffle();
	std::vector<DefaultMockAgentBase<8>*> in_1_v {in_1.begin(), in_1.end()};
	fpmas::model::Neighbors<DefaultMockAgentBase<8>> in_2 = in.shuffle();
	std::vector<DefaultMockAgentBase<8>*> in_2_v {in_2.begin(), in_2.end()};
	fpmas::model::Neighbors<DefaultMockAgentBase<8>> in_3 = in.shuffle();
	std::vector<DefaultMockAgentBase<8>*> in_3_v {in_3.begin(), in_3.end()};

	ASSERT_THAT(in_1_v, Not(ElementsAreArray(in_v)));
	ASSERT_THAT(in_2_v, Not(ElementsAreArray(in_v)));
	ASSERT_THAT(in_3_v, Not(ElementsAreArray(in_v)));

	ASSERT_THAT(in_2_v, Not(ElementsAreArray(in_1_v)));
	ASSERT_THAT(in_3_v, Not(ElementsAreArray(in_2_v)));
	ASSERT_THAT(in_3_v, Not(ElementsAreArray(in_1_v)));

	for(auto neighbor : in_neighbors)
		delete neighbor;
}

class FakeAgent : public MockAgentBase<FakeAgent> {
	public:
		int field = 0;
		FakeAgent(int field) : field(field) {
		}
};

class AgentGuardTest : public ::testing::Test {
	protected:
		FakeAgent* agent = new FakeAgent(0);
		AgentPtr ptr {agent};
		AgentTask task {ptr};
		MockDistributedNode<AgentPtr> node {{0, 0}, std::move(ptr)};
		MockMutex<AgentPtr> mutex;

		void SetUp() {
			agent->setNode(&node);
			agent->setTask(&task);
			EXPECT_CALL(node, mutex()).Times(AnyNumber())
				.WillRepeatedly(Return(&mutex));
		}
};

class MoveAgent {
	private:
		AgentPtr& data;
		AgentPtr& data_to_move;

	public:
		MoveAgent(AgentPtr& data, AgentPtr& data_to_move)
			: data(data), data_to_move(data_to_move) {}

		void operator()() {
			data = std::move(data_to_move);
		}

};

TEST_F(AgentGuardTest, read_guard) {
	AgentPtr& ptr = node.data();
	AgentPtr data_to_move {new FakeAgent(14)};

	EXPECT_CALL(mutex, read).WillOnce(DoAll(InvokeWithoutArgs(MoveAgent(ptr, data_to_move)), ReturnRef(node.data())));
	const fpmas::model::ReadGuard read(ptr);

	// Pointed data has implicitely been updated by mutex::read
	ASSERT_EQ(static_cast<FakeAgent*>(ptr.get())->field, 14);

	EXPECT_CALL(mutex, releaseRead);
}

TEST_F(AgentGuardTest, acquire_guard) {
	AgentPtr& ptr = node.data();
	AgentPtr data_to_move {new FakeAgent(14)};

	EXPECT_CALL(mutex, acquire).WillOnce(DoAll(InvokeWithoutArgs(MoveAgent(ptr, data_to_move)), ReturnRef(node.data())));
	const fpmas::model::AcquireGuard acq(ptr);

	// Pointed data has implicitely been updated by mutex::acquire
	ASSERT_EQ(static_cast<FakeAgent*>(ptr.get())->field, 14);

	EXPECT_CALL(mutex, releaseAcquire);
}

class AgentGuardLoopTest : public AgentGuardTest {
	protected:
		FakeAgent* agent_1 = new FakeAgent(0);
		MockDistributedNode<AgentPtr> node_1 {{0, 0}, agent_1};
		MockMutex<AgentPtr> mutex_1;
		AgentPtr& agent_ptr_1 = node_1.data();
		AgentPtr data_to_move_1 {new FakeAgent(14)};

		FakeAgent* agent_2 = new FakeAgent(0);
		MockDistributedNode<AgentPtr> node_2 {{0, 0}, agent_2};
		MockMutex<AgentPtr> mutex_2;
		AgentPtr& agent_ptr_2 = node_2.data();
		AgentPtr data_to_move_2 {new FakeAgent(14)};

		void SetUp() override {
			AgentGuardTest::SetUp();
			agent_1->setNode(&node_1);
			agent_1->setTask(&task); // Don't care about task, all agents use the same
			EXPECT_CALL(node_1, mutex()).Times(AnyNumber())
				.WillRepeatedly(Return(&mutex_1));

			AgentGuardTest::SetUp();
			agent_2->setNode(&node_2);
			agent_2->setTask(&task); // Don't care about task, all agents use the same
			EXPECT_CALL(node_2, mutex()).Times(AnyNumber())
				.WillRepeatedly(Return(&mutex_2));

			EXPECT_CALL(node, outNeighbors()).Times(AnyNumber())
				.WillRepeatedly(Return(
							std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*>
							{&node_1, &node_2}
							));
		}
};

TEST_F(AgentGuardLoopTest, neighbor_loop_read) {
	EXPECT_CALL(mutex_1, read).WillOnce(DoAll(InvokeWithoutArgs(
					MoveAgent(agent_ptr_1, data_to_move_1)),
				ReturnRef(node.data())));
	EXPECT_CALL(mutex_2, read).WillOnce(DoAll(InvokeWithoutArgs(
					MoveAgent(agent_ptr_2, data_to_move_2)),
				ReturnRef(node.data())));

	EXPECT_CALL(mutex_1, releaseRead);
	EXPECT_CALL(mutex_2, releaseRead);

	auto neighbors = agent->outNeighbors<FakeAgent>();
	for(fpmas::model::Neighbor<FakeAgent>& fake_agent : neighbors) {
		fpmas::model::ReadGuard read(fake_agent);
		ASSERT_EQ(fake_agent->field, 14);
	}
}

TEST_F(AgentGuardLoopTest, neighbor_loop_acquire) {
	EXPECT_CALL(mutex_1, acquire).WillOnce(DoAll(InvokeWithoutArgs(
					MoveAgent(agent_ptr_1, data_to_move_1)),
				ReturnRef(node.data())));
	EXPECT_CALL(mutex_2, acquire).WillOnce(DoAll(InvokeWithoutArgs(
					MoveAgent(agent_ptr_2, data_to_move_2)),
				ReturnRef(node.data())));

	EXPECT_CALL(mutex_1, releaseAcquire);
	EXPECT_CALL(mutex_2, releaseAcquire);

	auto neighbors = agent->outNeighbors<FakeAgent>();
	for(fpmas::model::Neighbor<FakeAgent>& fake_agent : neighbors) {
		fpmas::model::AcquireGuard acquire(fake_agent);
		ASSERT_EQ(fake_agent->field, 14);
	}
}
