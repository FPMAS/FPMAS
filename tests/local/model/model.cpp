#include "fpmas.h"

#include "fpmas/graph/distributed_graph.h"
#include "fpmas/model/model.h"
#include "graph/mock_distributed_edge.h"
#include "graph/mock_distributed_node.h"
#include "graph/mock_distributed_graph.h"
#include "graph/mock_load_balancing.h"
#include "model/mock_model.h"
#include "communication/mock_communication.h"
#include "synchro/mock_mutex.h"
#include "io/mock_breakpoint.h"
#include "scheduler/mock_scheduler.h"
#include "runtime/mock_runtime.h"
#include "synchro/mock_sync_mode.h"

using namespace testing;

using fpmas::model::detail::Model;
using fpmas::model::detail::AgentGroup;
using fpmas::model::detail::AgentTask;

typedef fpmas::api::model::AgentPtr AgentPtr;

class AgentTaskTest : public ::testing::Test {
	protected:
		MockAgent<> agent;
		AgentPtr agent_ptr {&agent};
		MockDistributedNode<AgentPtr, NiceMock> node;

		void SetUp() override {
			ON_CALL(agent, node())
				.WillByDefault(Return(&node));
			ON_CALL(Const(agent), node())
				.WillByDefault(Return(&node));
		}

		void TearDown() override {
			agent_ptr.release();
		}

};

TEST_F(AgentTaskTest, build) {
	AgentTask agent_task(agent_ptr);

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
		MockMpiCommunicator<> mock_comm;
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
		fpmas::api::utils::EventCallback<fpmas::api::graph::SetLocalNodeEvent<AgentPtr>>* set_local_callback;
		fpmas::api::utils::EventCallback<fpmas::api::graph::SetDistantNodeEvent<AgentPtr>>* set_distant_callback;

		Model* model;
		void SetUp() override {
			EXPECT_CALL(graph, getMpiCommunicator())
				.Times(AnyNumber())
				.WillRepeatedly(ReturnRef(mock_comm));
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

TEST_F(ModelTest, remove_group) {
	FPMAS_DEFINE_GROUPS(G_1, G_2);

	// Absurdly complex set up to simulate group.add(agent) and
	// group.remove(agent)...
	// It would really be better if we could use MockAgentGroup in Model, and
	// simply expect mock_agent_group.clear()
	struct MockBuildNode {
		MockAgentNode<NiceMock> mock_node;
		fpmas::api::model::AgentTask* task;
		std::unordered_map<
			fpmas::api::model::GroupId,
			fpmas::api::model::AgentTask*> task_list;

		void setTask(Unused, fpmas::api::model::AgentTask* task) {
			this->task = task;
			task_list[G_1] = task;
		}
		MockBuildNode(MockAgent<>* agent)
			: mock_node({0, 0}, agent), task_list({{G_1, task}}) {
				ON_CALL(mock_node, state())
					.WillByDefault(Return(fpmas::api::graph::LOCAL));
				ON_CALL(*agent, node())
					.WillByDefault(Return(&mock_node));
				ON_CALL(mock_node, state)
					.WillByDefault(Return(fpmas::api::graph::LOCAL));
				ON_CALL(*agent, setTask(G_1, _))
					.WillByDefault(Invoke(this, &MockBuildNode::setTask));
				ON_CALL(*agent, task(G_1))
					.WillByDefault(ReturnPointee(&task));
				ON_CALL(*agent, tasks)
					.WillByDefault(ReturnRef(task_list));
			}

		fpmas::api::graph::DistributedNode<AgentPtr>* build(fpmas::api::model::AgentPtr& ptr) {
			ptr.release();
			return &mock_node;
		}
	};

	MockAgent<>* agent = new MockAgent<>;
	MockBuildNode mock_build_node(agent);
	EXPECT_CALL(graph, buildNode_rv)
		.WillOnce(Invoke(&mock_build_node, &MockBuildNode::build));

	auto& group_1 = model->buildGroup(G_1);
	auto& group_2 = model->buildGroup(G_2);

	std::vector<fpmas::api::model::GroupId> group_ids {G_1};
	std::vector<fpmas::api::model::AgentGroup*> groups {&group_1};
	std::list<fpmas::api::model::Agent*>::iterator group_pos;
	group_1.add(agent);
	ON_CALL(*agent, groupIds)
		.WillByDefault(ReturnPointee(&group_ids));
	ON_CALL(*agent, setGroupPos(G_1, _))
		.WillByDefault(SaveArg<1>(&group_pos));
	ON_CALL(*agent, getGroupPos(G_1))
		.WillByDefault(ReturnPointee(&group_pos));
	ON_CALL(*agent, groups())
		.WillByDefault(ReturnPointee(&groups));
	insert_node_callback->call(&mock_build_node.mock_node);
	set_local_callback->call({
			&mock_build_node.mock_node,
			fpmas::api::model::SetAgentLocalEvent::BUILD_LOCAL
			});

	// Expect group_1.remove(agent)
	// It would be more efficient to mock group_1 and group_2 but this is
	// currently not possible
	EXPECT_CALL(*agent, removeGroup(&group_1))
		.WillOnce(InvokeWithoutArgs(&groups, &decltype(groups)::clear));
	EXPECT_CALL(*agent, removeGroupId(G_1))
		.WillOnce(InvokeWithoutArgs(&group_ids, &decltype(group_ids)::clear));
	EXPECT_CALL(graph, removeNode(&mock_build_node.mock_node))
		.WillOnce(Invoke(erase_node_callback, &NodeCallback::call));

	model->removeGroup(group_1);
	ASSERT_THAT(model->groups(), ElementsAre(Pair(G_2, &group_2)));
}

TEST_F(ModelTest, load_balancing_job) {
	scheduler.schedule(0, 1, model->loadBalancingJob());
	
	MockMpiCommunicator<4, 10> mock_comm;
	EXPECT_CALL(graph, getMpiCommunicator()).Times(AnyNumber())
		.WillRepeatedly(ReturnRef(mock_comm));
	EXPECT_CALL(graph, getNodes).Times(AnyNumber());
	EXPECT_CALL(graph, getEdges).Times(AnyNumber());

	{
		InSequence s;
		EXPECT_CALL(graph, balance(Ref(load_balancing), fpmas::api::graph::PARTITION));
		EXPECT_CALL(graph, balance(Ref(load_balancing), fpmas::api::graph::REPARTITION));
		EXPECT_CALL(graph, balance(Ref(load_balancing), fpmas::api::graph::REPARTITION));
	}
	runtime.run(3);
}

TEST_F(ModelTest, link) {
	MockAgent<0> agent_1;
	MockAgentNode<NiceMock> node_1;
	ON_CALL(agent_1, node()).WillByDefault(Return(&node_1));

	MockAgentNode<NiceMock> node_2;
	MockAgent<4> agent_2;
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
		NiceMock<MockModel> model;
		fpmas::model::detail::InsertAgentNodeCallback insert_agent_callback {model};
		fpmas::model::detail::EraseAgentNodeCallback erase_agent_callback {model};
		fpmas::model::detail::SetAgentLocalCallback set_local_callback {model};
		fpmas::model::detail::SetAgentDistantCallback set_distant_callback {model};

		fpmas::api::model::GroupId id = 10;

		MockDistributedGraph<
			AgentPtr, MockDistributedNode<AgentPtr>, MockDistributedEdge<AgentPtr>,
			NiceMock>
			graph;
		// Mock communicator for the Runtime
		MockMpiCommunicator<4, 10> mock_comm;
		MockDistributedNode<AgentPtr, NiceMock> node1 {fpmas::graph::DistributedId(0, 1)};
		MockDistributedNode<AgentPtr, NiceMock> node2 {fpmas::graph::DistributedId(0, 2)};

		AgentGroup agent_group {id, graph};
		MockAgent<> agent1;
		fpmas::api::model::AgentNode* agent1_node;
		std::vector<fpmas::api::model::AgentGroup*> agent1_groups;
		std::vector<fpmas::api::model::GroupId> agent1_group_ids;
		std::unordered_map<
			fpmas::api::model::GroupId,
			std::list<fpmas::api::model::Agent*>::iterator
				> agent1_group_pos;
		std::unordered_map<fpmas::model::GroupId, fpmas::api::model::AgentTask*> agent1_tasks;
		AgentPtr agent1_ptr {&agent1};

		MockAgent<> agent2;
		fpmas::api::model::AgentNode* agent2_node;
		std::vector<fpmas::api::model::AgentGroup*> agent2_groups;
		std::vector<fpmas::api::model::GroupId> agent2_group_ids;
		std::unordered_map<
			fpmas::api::model::GroupId,
			std::list<fpmas::api::model::Agent*>::iterator
				> agent2_group_pos;
		std::unordered_map<fpmas::model::GroupId, fpmas::api::model::AgentTask*> agent2_tasks;
		AgentPtr agent2_ptr {&agent2};

		fpmas::scheduler::Scheduler scheduler;
		fpmas::runtime::Runtime runtime {scheduler};

		void SetUp() override {
			fpmas::runtime::Runtime::distributed_rd = {mock_comm};
			// In case of LOG
			ON_CALL(model, graph())
				.WillByDefault(ReturnRef(graph));
			ON_CALL(Const(model), graph())
				.WillByDefault(ReturnRef(graph));
			ON_CALL(model, getMpiCommunicator())
				.WillByDefault(ReturnRef(mock_comm));
			ON_CALL(graph, getMpiCommunicator())
				.WillByDefault(ReturnRef(mock_comm));

			// Node data set up
			ON_CALL(node1, data())
				.WillByDefault(ReturnRef(agent1_ptr));
			ON_CALL(agent1, node())
				.WillByDefault(ReturnPointee(&agent1_node));
			ON_CALL(agent1, setNode)
				.WillByDefault(SaveArg<0>(&agent1_node));
			ON_CALL(node2, data())
				.WillByDefault(ReturnRef(agent2_ptr));
			ON_CALL(agent2, node())
				.WillByDefault(ReturnPointee(&agent2_node));
			ON_CALL(agent2, setNode)
				.WillByDefault(SaveArg<0>(&agent2_node));

			// Group Ids set up
			ON_CALL(agent1, groupIds())
				.WillByDefault(ReturnPointee(&agent1_group_ids));
			ON_CALL(agent1, addGroupId).WillByDefault(
					[this] (fpmas::api::model::GroupId id) {
						agent1_group_ids.push_back(id);
						});
			ON_CALL(agent1, removeGroupId).WillByDefault(
					[this] (fpmas::api::model::GroupId id) {
						agent1_group_ids.erase(std::remove(agent1_group_ids.begin(), agent1_group_ids.end(), id));
						});

			ON_CALL(agent2, groupIds())
				.WillByDefault(ReturnPointee(&agent2_group_ids));
			ON_CALL(agent2, addGroupId).WillByDefault(
					[this] (fpmas::api::model::GroupId id) {
						agent2_group_ids.push_back(id);
						});
			ON_CALL(agent2, removeGroupId).WillByDefault(
					[this] (fpmas::api::model::GroupId id) {
						agent2_group_ids.erase(std::remove(agent2_group_ids.begin(), agent2_group_ids.end(), id));
						});

			// Group pos set up
			ON_CALL(agent1, setGroupPos).WillByDefault(
					[this] (
						fpmas::api::model::GroupId id,
						std::list<fpmas::api::model::Agent*>::iterator pos) {
					agent1_group_pos[id] = pos;
					});
			ON_CALL(agent1, getGroupPos).WillByDefault(
					[this] (fpmas::api::model::GroupId id) {
					return agent1_group_pos.find(id)->second;
					});
			ON_CALL(agent2, setGroupPos).WillByDefault(
					[this] (
						fpmas::api::model::GroupId id,
						std::list<fpmas::api::model::Agent*>::iterator pos) {
					agent2_group_pos[id] = pos;
					});
			ON_CALL(agent2, getGroupPos).WillByDefault(
					[this] (fpmas::api::model::GroupId id) {
					return agent2_group_pos.find(id)->second;
					});

			// Task set up
			ON_CALL(agent1, setTask(_, _)).WillByDefault(
					[this] (fpmas::model::GroupId id, fpmas::api::model::AgentTask* task) {
						agent1_tasks[id] = task;
						});
			ON_CALL(agent1, task(_)).WillByDefault(
					[this] (fpmas::model::GroupId id) {
						return agent1_tasks[id];
						});
			ON_CALL(agent1, tasks())
				.WillByDefault(ReturnPointee(&agent1_tasks));

			ON_CALL(agent2, setTask(_, _)).WillByDefault(
					[this] (fpmas::model::GroupId id, fpmas::api::model::AgentTask* task) {
						agent2_tasks[id] = task;
						});
			ON_CALL(agent2, task(_)).WillByDefault(
					[this] (fpmas::model::GroupId id) {
						return agent2_tasks[id];
						});
			ON_CALL(agent2, tasks())
				.WillByDefault(ReturnPointee(&agent2_tasks));

			// AgentGroup set up
			ON_CALL(agent1, groups())
				.WillByDefault(ReturnPointee(&agent1_groups));
			ON_CALL(agent1, addGroup).WillByDefault(
					[this] (fpmas::api::model::AgentGroup* group) {
						agent1_groups.push_back(group);
						});
			ON_CALL(agent1, removeGroup).WillByDefault(
					[this] (fpmas::api::model::AgentGroup* group) {
						agent1_groups.erase(std::remove(agent1_groups.begin(), agent1_groups.end(), group));
						});
			ON_CALL(agent2, groups())
				.WillByDefault(ReturnPointee(&agent2_groups));
			ON_CALL(agent2, addGroup).WillByDefault(
					[this] (fpmas::api::model::AgentGroup* group) {
						agent2_groups.push_back(group);
						});
			ON_CALL(agent2, removeGroup).WillByDefault(
					[this] (fpmas::api::model::AgentGroup* group) {
						agent2_groups.erase(std::remove(agent2_groups.begin(), agent2_groups.end(), group));
						});

			ON_CALL(graph, insert(A<Node*>()))
				.WillByDefault(DoAll(
							Invoke(&insert_agent_callback, &fpmas::model::detail::InsertAgentNodeCallback::call),
							Invoke([this] (Node* node) {
								set_local_callback.call(fpmas::api::graph::SetLocalNodeEvent<AgentPtr>(
											node, fpmas::api::graph::SetLocalNodeEvent<AgentPtr>::BUILD_LOCAL));
								})
							));
			ON_CALL(graph, erase(A<Node*>()))
				.WillByDefault(DoAll(
							Invoke(&erase_agent_callback, &fpmas::model::detail::EraseAgentNodeCallback::call)
							));

			ON_CALL(model, getGroup(id))
				.WillByDefault(ReturnRef(agent_group));
		}

		void AddAgentsToGroup() {
			// Agent 1 set up
			EXPECT_CALL(graph, buildNode_rv)
				.WillOnce([this] (fpmas::model::AgentPtr& ptr) {
						graph.insert(&node1);
						ptr.release();
						return &node1;
						});
			EXPECT_CALL(agent1, setNode(&node1));
			EXPECT_CALL(agent1, setModel(&model));
			EXPECT_CALL(agent1, addGroup(&agent_group));
			EXPECT_CALL(agent1, addGroupId(id));
			EXPECT_CALL(agent1, setGroupPos(
						id,
						ResultOf([] (
								std::list<fpmas::api::model::Agent*>::iterator pos
								) {return *pos;}, &agent1
							))
					).Times(AnyNumber());
			EXPECT_CALL(agent1, setTask(id, _));
			agent_group.add(&agent1);

			// Agent 2 set up
			EXPECT_CALL(graph, buildNode_rv)
				.WillOnce([this] (fpmas::model::AgentPtr& ptr) {
						graph.insert(&node2);
						ptr.release();
						return &node2;
						});
			EXPECT_CALL(agent2, setNode(&node2));
			EXPECT_CALL(agent2, setModel(&model));
			EXPECT_CALL(agent2, addGroup(&agent_group));
			EXPECT_CALL(agent2, addGroupId(id));
			EXPECT_CALL(agent2, setGroupPos(
						id,
						ResultOf([] (
								std::list<fpmas::api::model::Agent*>::iterator pos
								) {return *pos;}, &agent2
							))
					).Times(AnyNumber());
			EXPECT_CALL(agent2, setTask(id, _));
			agent_group.add(&agent2);

			using fpmas::api::graph::DistributedNode;
			ON_CALL(graph, removeNode(A<DistributedNode<AgentPtr>*>()))
				.WillByDefault(Invoke(
							&erase_agent_callback,
							&fpmas::model::detail::EraseAgentNodeCallback::call
							));
		}

		void EraseNode(MockAgent<>& agent) {
			for(auto group : agent.groups()) {
				EXPECT_CALL(agent, removeGroup(group));
				EXPECT_CALL(agent, removeGroupId(group->groupId()));
			}
			// Would normally be called from the Graph destructor
			erase_agent_callback.call(agent.node());
		}

		void TearDown() override {
			fpmas::runtime::Runtime::distributed_rd = {};
			agent1_ptr.release();
			agent2_ptr.release();
		}
};

TEST_F(AgentGroupTest, id) {
	ASSERT_EQ(agent_group.groupId(), 10);
}

TEST_F(AgentGroupTest, job_end) {
	EXPECT_CALL(graph, synchronize());

	agent_group.job().getEndTask().run();
}

TEST_F(AgentGroupTest, add_agent) {
	AddAgentsToGroup();

	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(&agent1, &agent2));
	ASSERT_THAT(
			agent_group.agentExecutionJob().tasks(),
			UnorderedElementsAre(agent1.task(id), agent2.task(id)));

	EraseNode(agent1);
	EraseNode(agent2);
}

TEST_F(AgentGroupTest, add_agent_to_an_other_group) {
	AgentGroup other_agent_group {id+1, graph};
	AddAgentsToGroup();

	EXPECT_CALL(agent2, addGroup(&other_agent_group));
	EXPECT_CALL(agent2, addGroupId(11));
	EXPECT_CALL(agent2, setGroupPos(11, _)).Times(AnyNumber());
	EXPECT_CALL(agent2, setTask(11, _));
	EXPECT_CALL(graph, buildNode_rv).Times(0);
	other_agent_group.add(&agent2);

	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(&agent1, &agent2));
	ASSERT_THAT(
			agent_group.agentExecutionJob().tasks(),
			UnorderedElementsAre(agent1.task(id), agent2.task(id)));
	ASSERT_THAT(other_agent_group.agents(), ElementsAre(&agent2));
	ASSERT_THAT(
			other_agent_group.agentExecutionJob().tasks(),
			ElementsAre(agent2.task(11)));

	EraseNode(agent1);
	EraseNode(agent2);
}

TEST_F(AgentGroupTest, remove_agent_from_last_group) {
	AddAgentsToGroup();

	EXPECT_CALL(agent1, node())
		.WillRepeatedly(Return(&node1));

	EXPECT_CALL(agent1, removeGroup(&agent_group));
	EXPECT_CALL(agent1, removeGroupId(10));
	EXPECT_CALL(graph, removeNode(&node1));

	agent_group.remove(&agent1);

	ASSERT_THAT(agent_group.agents(), ElementsAre(&agent2));
	ASSERT_THAT(
			agent_group.agentExecutionJob().tasks(),
			ElementsAre(agent2.task(id)));

	EraseNode(agent2);
}

TEST_F(AgentGroupTest, remove_agent_not_last_group) {
	AgentGroup other_agent_group {id+1, graph};
	AddAgentsToGroup();

	EXPECT_CALL(agent1, node())
		.WillRepeatedly(Return(&node1));

	EXPECT_CALL(agent1, addGroup(&other_agent_group));
	EXPECT_CALL(agent1, addGroupId(11));
	EXPECT_CALL(agent1, setGroupPos(11, _)).Times(AnyNumber());
	EXPECT_CALL(agent1, setTask(11, _));
	other_agent_group.add(&agent1);

	EXPECT_CALL(agent1, removeGroup(&agent_group))
		.RetiresOnSaturation();
	EXPECT_CALL(agent1, removeGroupId(10))
		.RetiresOnSaturation();
	EXPECT_CALL(graph, removeNode(&node1)).Times(0);

	agent_group.remove(&agent1);

	ASSERT_THAT(agent_group.agents(), ElementsAre(&agent2));
	ASSERT_THAT(
			agent_group.agentExecutionJob().tasks(),
			ElementsAre(agent2.task(id)));

	EraseNode(agent1);
	EraseNode(agent2);
}

TEST_F(AgentGroupTest, clear) {
	AddAgentsToGroup();

	EXPECT_CALL(graph, removeNode(&node1));
	EXPECT_CALL(graph, removeNode(&node2));

	agent_group.clear();
	ASSERT_THAT(agent_group.agents(), IsEmpty());
	ASSERT_THAT(agent_group.agentExecutionJob().tasks(), IsEmpty());
}

TEST_F(AgentGroupTest, insert_agent) {
	// Nodes are assumed to have been inserted into the graph
	agent1.setNode(&node1);
	insert_agent_callback.call(&node1);
	agent2.setNode(&node2);
	insert_agent_callback.call(&node2);

	EXPECT_CALL(agent1, addGroup(&agent_group));
	EXPECT_CALL(agent1, setTask(id, _));
	agent_group.insert(&agent1_ptr);

	EXPECT_CALL(agent2, addGroup(&agent_group));
	EXPECT_CALL(agent2, setTask(id, _));
	agent_group.insert(&agent2_ptr);

	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(&agent1, &agent2));

	delete agent1.task(id);
	delete agent2.task(id);
}

TEST_F(AgentGroupTest, erase_agent) {
	// Nodes are assumed to have been inserted into the graph
	agent1.setNode(&node1);
	insert_agent_callback.call(&node1);
	agent2.setNode(&node2);
	insert_agent_callback.call(&node2);

	// It is not the responsability of insert to call addGroupId():
	// agent.groupIds() is always assumed to already contain group.groupId()
	// before the agent is inserted into group.
	agent1.addGroupId(id);
	agent_group.insert(&agent1_ptr);
	agent2.addGroupId(id);
	agent_group.insert(&agent2_ptr);

	EXPECT_CALL(agent1, removeGroup(&agent_group));
	EXPECT_CALL(agent1, removeGroupId(id));
	agent_group.erase(&agent1_ptr);
	ASSERT_THAT(agent_group.agents(), UnorderedElementsAre(&agent2));

	EXPECT_CALL(agent2, removeGroup(&agent_group));
	EXPECT_CALL(agent2, removeGroupId(id));
	agent_group.erase(&agent2_ptr);
	ASSERT_THAT(agent_group.agents(), IsEmpty());
}

TEST_F(AgentGroupTest, agent_task) {
	AddAgentsToGroup();

	ASSERT_THAT(agent_group.agentExecutionJob().tasks(), SizeIs(2));

	EXPECT_CALL(graph, synchronize());
	::testing::Sequence s1, s2;

	EXPECT_CALL(agent1, act)
		.InSequence(s1);

	EXPECT_CALL(agent2, act)
		.InSequence(s2);

	runtime.execute(agent_group.agentExecutionJob());

	// Would normally be called from the Graph destructor
	erase_agent_callback.call(&node1);
	erase_agent_callback.call(&node2);
}

TEST_F(AgentGroupTest, local_agents) {
	// Agent 1 set up
	agent1.setNode(&node1);
	insert_agent_callback.call(&node1);
	agent1.addGroupId(id);
	agent_group.insert(&agent1_ptr);

	// Agent 2 set up
	agent2.setNode(&node2);
	insert_agent_callback.call(&node2);
	agent2.addGroupId(id);
	agent_group.insert(&agent2_ptr);

	ON_CALL(node1, state)
		.WillByDefault(Return(LocationState::LOCAL));
	set_local_callback.call({
			&node1, 
			fpmas::api::model::SetAgentLocalEvent::IMPORT_NEW_LOCAL
			});
	ON_CALL(node2, state)
		.WillByDefault(Return(LocationState::LOCAL));
	set_local_callback.call({
			&node2, 
			fpmas::api::model::SetAgentLocalEvent::IMPORT_NEW_LOCAL
			});

	ASSERT_THAT(agent_group.localAgents(), UnorderedElementsAre(&agent1, &agent2));

	ON_CALL(node1, state)
		.WillByDefault(Return(LocationState::DISTANT));
	set_distant_callback.call({
			&node1,
			fpmas::api::model::SetAgentDistantEvent::EXPORT_DISTANT
			});

	ASSERT_THAT(agent_group.localAgents(), ElementsAre(&agent2));

	ON_CALL(node2, state)
		.WillByDefault(Return(LocationState::DISTANT));
	set_distant_callback.call({
			&node2,
			fpmas::api::model::SetAgentDistantEvent::EXPORT_DISTANT
			});

	ASSERT_THAT(agent_group.localAgents(), IsEmpty());

	EraseNode(agent1);
	EraseNode(agent2);
}

TEST_F(AgentGroupTest, distant_agents) {
	// Agent 1 set up
	agent1.setNode(&node1);
	insert_agent_callback.call(&node1);
	agent1.addGroupId(id);
	agent_group.insert(&agent1_ptr);

	// Agent 2 set up
	agent2.setNode(&node2);
	insert_agent_callback.call(&node2);
	agent2.addGroupId(id);
	agent_group.insert(&agent2_ptr);

	ON_CALL(node1, state)
		.WillByDefault(Return(LocationState::DISTANT));
	set_distant_callback.call({
			&node1,
			fpmas::api::model::SetAgentDistantEvent::IMPORT_NEW_DISTANT
			});
	ON_CALL(node2, state)
		.WillByDefault(Return(LocationState::DISTANT));
	set_distant_callback.call({
			&node2,
			fpmas::api::model::SetAgentDistantEvent::IMPORT_NEW_DISTANT
			});

	ASSERT_THAT(agent_group.distantAgents(), UnorderedElementsAre(&agent1, &agent2));

	ON_CALL(node1, state)
		.WillByDefault(Return(LocationState::LOCAL));
	set_local_callback.call({
			&node1, 
			fpmas::api::model::SetAgentLocalEvent::IMPORT_NEW_LOCAL
			});

	ASSERT_THAT(agent_group.distantAgents(), ElementsAre(&agent2));

	ON_CALL(node2, state)
		.WillByDefault(Return(LocationState::LOCAL));
	set_local_callback.call({
			&node2, 
			fpmas::api::model::SetAgentLocalEvent::IMPORT_NEW_LOCAL
			});

	ASSERT_THAT(agent_group.distantAgents(), IsEmpty());

	EraseNode(agent1);
	EraseNode(agent2);
}

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

TEST_F(AgentBaseTest, group_ids) {
	agent_base.addGroupId(12);
	agent_base.addGroupId(4);
	agent_base.addGroupId(45);
	ASSERT_THAT(agent_base.groupIds(), UnorderedElementsAre(12, 4, 45));
}

TEST_F(AgentBaseTest, remove_group_id) {
	agent_base.addGroupId(12);
	agent_base.addGroupId(4);
	agent_base.addGroupId(45);

	agent_base.removeGroupId(4);

	ASSERT_THAT(agent_base.groupIds(), UnorderedElementsAre(12, 45));
}


TEST_F(AgentBaseTest, groups) {
	MockAgentGroup group1;
	MockAgentGroup group2;
	MockAgentGroup group3;

	agent_base.addGroup(&group1);
	agent_base.addGroup(&group2);
	agent_base.addGroup(&group3);

	ASSERT_THAT(
			agent_base.groups(),
			UnorderedElementsAre(&group1, &group2, &group3));
	ASSERT_THAT(
			const_agent_base.groups(),
			UnorderedElementsAre(&group1, &group2, &group3));
}

TEST_F(AgentBaseTest, remove_group) {
	AgentPtr ptr(&agent_base);
	AgentTask t1(ptr);
	AgentTask t2(ptr);
	AgentTask t3(ptr);

	NiceMock<MockAgentGroup> group1;
	NiceMock<MockAgentGroup> group2;
	NiceMock<MockAgentGroup> group3;

	ON_CALL(group1, groupId)
		.WillByDefault(Return(1));
	ON_CALL(group2, groupId)
		.WillByDefault(Return(2));
	ON_CALL(group3, groupId)
		.WillByDefault(Return(3));

	agent_base.addGroup(&group1);
	agent_base.setTask(1, &t1);
	agent_base.addGroup(&group2);
	agent_base.setTask(2, &t2);
	agent_base.addGroup(&group3);
	agent_base.setTask(3, &t3);


	agent_base.removeGroup(&group2);
	ASSERT_THAT(
			agent_base.groups(),
			UnorderedElementsAre(&group1, &group3));
	ASSERT_THAT(
			const_agent_base.groups(),
			UnorderedElementsAre(&group1, &group3));

	// Task associated to group 2 must have been removed
	ASSERT_THAT(
			agent_base.tasks(),
			UnorderedElementsAre(Pair(1, &t1), Pair(3, &t3)));

	ptr.release();
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

TEST_F(AgentBaseTest, tasks) {
	AgentPtr ptr(&agent_base);
	AgentTask t1(ptr);
	AgentTask t2(ptr);
	AgentTask t3(ptr);
	agent_base.setTask(1, &t1);
	agent_base.setTask(4, &t2);
	agent_base.setTask(2, &t3);
	ASSERT_EQ(agent_base.task(1), &t1);
	ASSERT_EQ(agent_base.task(4), &t2);
	ASSERT_EQ(agent_base.task(2), &t3);

	ASSERT_THAT(agent_base.tasks(),
			UnorderedElementsAre(Pair(1, &t1), Pair(4, &t2), Pair(2, &t3)));

	ptr.release();
}

class AgentBaseCopyMoveTest : public testing::Test {
	protected:
		class TestAgent : public fpmas::model::AgentBase<TestAgent> {
			public:
				const TestAgent* copy_constructed_from = nullptr;

				TestAgent() {}

				TestAgent(const TestAgent& agent) {
					copy_constructed_from = &agent;
				}

				MOCK_METHOD(void, copy_assign, (const TestAgent& agent), ());
				TestAgent& operator=(const TestAgent& agent) {
					copy_assign(agent);
					return *this;
				}

				MOCK_METHOD(void, move_assign, (const TestAgent& agent), ());
				TestAgent& operator=(TestAgent&& agent) {
					move_assign(agent);
					return *this;
				}

				MOCK_METHOD(void, act, (), (override));
		};

		TestAgent test_agent;
};

TEST_F(AgentBaseCopyMoveTest, copy) {
	fpmas::api::model::Agent* agent = test_agent.copy();

	ASSERT_THAT(agent, WhenDynamicCastTo<TestAgent*>(Not(IsNull())));
	ASSERT_EQ(dynamic_cast<TestAgent*>(agent)->copy_constructed_from, &test_agent);

	delete agent;
}

TEST_F(AgentBaseCopyMoveTest, copy_assign) {
	TestAgent agent_to_copy;

	EXPECT_CALL(test_agent, copy_assign(Ref(agent_to_copy)));

	test_agent.copyAssign(&agent_to_copy);
}

TEST_F(AgentBaseCopyMoveTest, move_assign) {
	TestAgent agent_to_move;
	AgentPtr ptr(&test_agent);

	MockAgentGroup g1;
	MockAgentGroup g2;
	AgentTask t1(ptr);
	AgentTask t2(ptr);
	AgentTask t3(ptr);

	MockModel mock_model;
	MockDistributedNode<AgentPtr> mock_node;
	test_agent.addGroup(&g1);
	test_agent.addGroup(&g2);
	test_agent.setTask(1, &t1);
	test_agent.setTask(12, &t2);
	test_agent.setTask(11, &t3);
	test_agent.setNode(&mock_node);
	test_agent.setModel(&mock_model);

	EXPECT_CALL(test_agent, move_assign(Ref(agent_to_move)));

	test_agent.moveAssign(&agent_to_move);

	ASSERT_EQ(test_agent.node(), &mock_node);
	ASSERT_EQ(test_agent.model(), &mock_model);
	ASSERT_THAT(test_agent.groups(), UnorderedElementsAre(&g1, &g2));
	ASSERT_THAT(test_agent.tasks(), UnorderedElementsAre(
				Pair(1, &t1), Pair(12, &t2), Pair(11, &t3)));

	ptr.release();
}

TEST_F(AgentBaseTest, out_neighbors) {
	MockAgentNode<NiceMock> n_1 {{0, 0}, new DefaultMockAgentBase<8>};
	MockAgentEdge<NiceMock> e_1;
	e_1.setTargetNode(&n_1);
	MockAgentNode<NiceMock> n_2 {{0, 1}, new DefaultMockAgentBase<12>};
	MockAgentEdge<NiceMock> e_2;
	e_2.setTargetNode(&n_2);
	MockAgentNode<NiceMock> n_3 {{0, 2}, new DefaultMockAgentBase<8>};
	MockAgentEdge<NiceMock> e_3;
	e_3.setTargetNode(&n_3);

	DefaultMockAgentBase<10>* agent = new DefaultMockAgentBase<10>;
	MockAgentNode<NiceMock> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*> out_neighbors {&n_1, &n_2, &n_3};
	ON_CALL(n, outNeighbors())
		.WillByDefault(Return(out_neighbors));
	std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*> out_edges {&e_1, &e_2, &e_3};
	ON_CALL(n, getOutgoingEdges())
		.WillByDefault(Return(out_edges));

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

TEST_F(AgentBaseTest, in_neighbors) {
	MockAgentNode<NiceMock> n_1 {{0, 0}, new DefaultMockAgentBase<8>};
	MockAgentEdge<NiceMock> e_1;
	e_1.setSourceNode(&n_1);
	MockAgentNode<NiceMock> n_2 {{0, 1}, new DefaultMockAgentBase<12>};
	MockAgentEdge<NiceMock> e_2;
	e_2.setSourceNode(&n_2);
	MockAgentNode<NiceMock> n_3 {{0, 2}, new DefaultMockAgentBase<8>};
	MockAgentEdge<NiceMock> e_3;
	e_3.setSourceNode(&n_3);

	DefaultMockAgentBase<10>* agent = new DefaultMockAgentBase<10>;
	MockAgentNode<NiceMock> n {{0, 4}, agent};
	std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*> in_neighbors {&n_1, &n_2, &n_3};
	std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*> in_edges {&e_1, &e_2, &e_3};
	ON_CALL(n, inNeighbors())
		.WillByDefault(Return(in_neighbors));
	ON_CALL(n, getIncomingEdges())
		.WillByDefault(Return(in_edges));

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


class FakeAgent : public MockAgentBase<FakeAgent> {
	public:
		int field = 0;
		FakeAgent(int field) : field(field) {
		}
};

class AgentGuardTest : public Test {
	protected:
		FakeAgent* agent = new FakeAgent(0);
		AgentPtr ptr {agent};
		AgentTask task {ptr};
		MockAgentNode<NiceMock> node {{0, 0}, std::move(ptr)};
		MockMutex<AgentPtr> mutex;

		void SetUp() {
			agent->setNode(&node);
			agent->setTask(&task);
			ON_CALL(node, mutex())
				.WillByDefault(Return(&mutex));
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

	EXPECT_CALL(mutex, read)
		.WillOnce(DoAll(
					InvokeWithoutArgs(MoveAgent(ptr, data_to_move)),
					ReturnRef(node.data())
					));
	const fpmas::model::ReadGuard read(ptr);

	// Pointed data has implicitely been updated by mutex::read
	ASSERT_EQ(dynamic_cast<FakeAgent*>(ptr.get())->field, 14);

	EXPECT_CALL(mutex, releaseRead);
}

TEST_F(AgentGuardTest, acquire_guard) {
	AgentPtr& ptr = node.data();
	AgentPtr data_to_move {new FakeAgent(14)};

	EXPECT_CALL(mutex, acquire)
		.WillOnce(DoAll(
					InvokeWithoutArgs(MoveAgent(ptr, data_to_move)),
					ReturnRef(node.data())
					));
	const fpmas::model::AcquireGuard acq(ptr);

	// Pointed data has implicitely been updated by mutex::acquire
	ASSERT_EQ(dynamic_cast<FakeAgent*>(ptr.get())->field, 14);

	EXPECT_CALL(mutex, releaseAcquire);
}

class AgentGuardLoopTest : public AgentGuardTest {
	protected:
		FakeAgent* agent_1 = new FakeAgent(0);
		MockAgentNode<NiceMock> node_1 {{0, 0}, agent_1};
		MockAgentEdge<NiceMock> edge_1;
		MockMutex<AgentPtr> mutex_1;
		AgentPtr& agent_ptr_1 = node_1.data();
		AgentPtr data_to_move_1 {new FakeAgent(14)};

		FakeAgent* agent_2 = new FakeAgent(0);
		MockAgentNode<NiceMock> node_2 {{0, 0}, agent_2};
		MockAgentEdge<NiceMock> edge_2;
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
			ON_CALL(node_2, mutex())
				.WillByDefault(Return(&mutex_2));

			edge_1.setTargetNode(&node_1);
			edge_2.setTargetNode(&node_2);
			ON_CALL(node, outNeighbors())
				.WillByDefault(Return(
							std::vector<fpmas::api::graph::DistributedNode<AgentPtr>*>
							{&node_1, &node_2}
							));
			ON_CALL(node, getOutgoingEdges())
				.WillByDefault(Return(
							std::vector<fpmas::api::graph::DistributedEdge<AgentPtr>*>
							{&edge_1, &edge_2}
							));

		}
};

TEST_F(AgentGuardLoopTest, neighbor_loop_read) {
	EXPECT_CALL(mutex_1, read)
		.WillOnce(DoAll(
					InvokeWithoutArgs(MoveAgent(agent_ptr_1, data_to_move_1)),
					ReturnRef(node.data())
					));
	EXPECT_CALL(mutex_2, read)
		.WillOnce(DoAll(
					InvokeWithoutArgs(
						MoveAgent(agent_ptr_2, data_to_move_2)),
					ReturnRef(node.data())
					));

	EXPECT_CALL(mutex_1, releaseRead);
	EXPECT_CALL(mutex_2, releaseRead);

	auto neighbors = agent->outNeighbors<FakeAgent>();
	for(fpmas::model::Neighbor<FakeAgent>& fake_agent : neighbors) {
		fpmas::model::ReadGuard read(fake_agent);
		ASSERT_EQ(fake_agent->field, 14);
	}
}

TEST_F(AgentGuardLoopTest, neighbor_loop_acquire) {
	EXPECT_CALL(mutex_1, acquire)
		.WillOnce(DoAll(
					InvokeWithoutArgs(MoveAgent(agent_ptr_1, data_to_move_1)),
					ReturnRef(node.data())
					));
	EXPECT_CALL(mutex_2, acquire)
		.WillOnce(DoAll(
					InvokeWithoutArgs(MoveAgent(agent_ptr_2, data_to_move_2)),
					ReturnRef(node.data())
					));

	EXPECT_CALL(mutex_1, releaseAcquire);
	EXPECT_CALL(mutex_2, releaseAcquire);

	auto neighbors = agent->outNeighbors<FakeAgent>();
	for(fpmas::model::Neighbor<FakeAgent>& fake_agent : neighbors) {
		fpmas::model::AcquireGuard acquire(fake_agent);
		ASSERT_EQ(fake_agent->field, 14);
	}
}

class AutoBreakpointTest : public Test {
	protected:
		MockMpiCommunicator<2, 10> comm;
		NiceMock<MockModel> model;
		MockBreakpoint<fpmas::api::model::Model> breakpoint;
		NiceMock<MockScheduler> scheduler;
		NiceMock<MockRuntime> mock_runtime;

		fpmas::model::AutoBreakpoint auto_bp {"test.%r.%t.msgpack", breakpoint, model};

		void SetUp() {
			ON_CALL(model, getMpiCommunicator())
				.WillByDefault(ReturnRef(comm));
			ON_CALL(Const(model), getMpiCommunicator())
				.WillByDefault(ReturnRef(comm));
			ON_CALL(model, scheduler())
				.WillByDefault(ReturnRef(scheduler));
			ON_CALL(model, runtime())
				.WillByDefault(ReturnRef(mock_runtime));
			ON_CALL(Const(model), scheduler())
				.WillByDefault(ReturnRef(scheduler));
			ON_CALL(Const(model), runtime())
				.WillByDefault(ReturnRef(mock_runtime));

			ON_CALL(mock_runtime, currentDate)
				.WillByDefault(Return(7.0f));
		}
};

TEST_F(AutoBreakpointTest, test) {
	fpmas::runtime::Runtime runtime(scheduler);
	std::ifstream file;

	// First output
	EXPECT_CALL(breakpoint, dump(_, Ref(model)))
		.WillOnce(Invoke([] (std::ostream& out, Unused) {
					out << "hello 1";
					}));

	runtime.execute(auto_bp.job());
	file.open("test.2.7.msgpack");
	ASSERT_TRUE(file.is_open());
	std::stringstream content_1;
	file >> content_1.rdbuf();
	ASSERT_EQ(content_1.str(), "hello 1");
	file.close();

	// An other output on the same file.
	// The previous content must be overriden
	EXPECT_CALL(breakpoint, dump(_, Ref(model)))
		.WillOnce(Invoke([] (std::ostream& out, Unused) {
					out << "hello 2";
					}));

	runtime.execute(auto_bp.job());
	file.open("test.2.7.msgpack");
	ASSERT_TRUE(file.is_open());
	std::stringstream content_2;
	file >> content_2.rdbuf();
	ASSERT_EQ(content_2.str(), "hello 2");
	file.close();

	// An other output on a different file
	ON_CALL(mock_runtime, currentDate)
		.WillByDefault(Return(12.0f));
	EXPECT_CALL(breakpoint, dump(_, Ref(model)))
		.WillOnce(Invoke([] (std::ostream& out, Unused) {
					out << "hello 3";
					}));


	runtime.execute(auto_bp.job());
	// The previous file still exists
	file.open("test.2.7.msgpack");
	ASSERT_TRUE(file.is_open());
	file.close();

	// Check new file
	file.open("test.2.12.msgpack");
	std::stringstream content_3;
	file >> content_3.rdbuf();
	ASSERT_EQ(content_3.str(), "hello 3");
	file.close();

	std::remove("test.2.7.msgpack");
	std::remove("test.2.12.msgpack");
}


template<typename T>
using NiceMockSyncMode = NiceMock<MockSyncMode<T>>;

TEST(DistributedAgentNodeBuilder, double_buildDistantNode) {
	NiceMock<MockMpiCommunicator<0, 4>> comm;

	fpmas::graph::DistributedGraph<fpmas::model::AgentPtr, NiceMockSyncMode>
		graph(comm);
	MockScheduler scheduler;
	MockRuntime runtime;
	MockLoadBalancing<fpmas::model::AgentPtr> lb;

	// Regiters insert/remove callbacks so that agents are properly added to
	// group
	fpmas::model::detail::Model model(
			graph, scheduler, runtime, lb
			);
	fpmas::model::IdleBehavior behavior;
	auto& group = model.buildGroup(1, behavior);

	fpmas::model::DistributedAgentNodeBuilder agent_node_builder(
			{group}, 1, [] () {return new DefaultMockAgentBase<>;}, comm);

	auto* first_node = agent_node_builder.buildDistantNode({0, 0}, 0, graph);
	// build same node with different location
	auto* second_node = agent_node_builder.buildDistantNode({0, 0}, 2, graph);

	// Consistency: no node is inserted the second time, and the returned node
	// must be the one already contained in the graph, as specified in the
	// DistributedGraph::insertDistant() method
	ASSERT_EQ(first_node, second_node);

	ASSERT_THAT(second_node->data()->groups(), ElementsAre(&group));
	ASSERT_THAT(second_node->location(), 2);
}
