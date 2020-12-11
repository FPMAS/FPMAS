#ifndef MOCK_MODEL_H
#define MOCK_MODEL_H
#include "gmock/gmock.h"
#include "../graph/mock_distributed_graph.h"
#include "../graph/mock_distributed_node.h"
#include "../graph/mock_distributed_edge.h"
#include "fpmas/api/model/model.h"
#include "fpmas/model/model.h"

using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::AnyNumber;
using ::testing::SaveArg;

namespace detail {
	template<typename Implem>
		class MockAgentBase : public virtual fpmas::api::model::Agent {
			public:
				static const fpmas::api::model::TypeId TYPE_ID;

				std::vector<fpmas::api::model::GroupId> gids;

				MOCK_METHOD(fpmas::api::model::GroupId, groupId, (), (const, override));
				MOCK_METHOD(std::vector<fpmas::api::model::GroupId>, groupIds, (), (const, override));
				MOCK_METHOD(void, setGroupId, (fpmas::api::model::GroupId), (override));
				MOCK_METHOD(void, addGroupId, (fpmas::api::model::GroupId), (override));
				MOCK_METHOD(void, removeGroupId, (fpmas::api::model::GroupId), (override));

				MOCK_METHOD(std::vector<fpmas::api::model::AgentGroup*>, groups, (), (override));
				MOCK_METHOD(fpmas::api::model::AgentGroup*, group, (), (override));
				MOCK_METHOD(std::vector<const fpmas::api::model::AgentGroup*>, groups, (), (const, override));
				MOCK_METHOD(const fpmas::api::model::AgentGroup*, group, (), (const, override));
				MOCK_METHOD(void, addGroup, (fpmas::api::model::AgentGroup*), (override));
				MOCK_METHOD(void, removeGroup, (fpmas::api::model::AgentGroup*), (override));
				MOCK_METHOD(void, setGroup, (fpmas::api::model::AgentGroup*), (override));

				MOCK_METHOD(fpmas::api::model::TypeId, typeId, (), (const, override));
				MOCK_METHOD(fpmas::api::model::Agent*, copy, (), (const, override));
				MOCK_METHOD(void, copyAssign, (fpmas::api::model::Agent*), (override));
				MOCK_METHOD(void, moveAssign, (fpmas::api::model::Agent*), (override));

				MOCK_METHOD(fpmas::api::model::AgentNode*, node, (), (override));
				MOCK_METHOD(const fpmas::api::model::AgentNode*, node, (), (const, override));
				MOCK_METHOD(void, setNode, (fpmas::api::model::AgentNode*), (override));

				MOCK_METHOD(fpmas::api::model::Model*, model, (), (override));
				MOCK_METHOD(const fpmas::api::model::Model*, model, (), (const, override));
				MOCK_METHOD(void, setModel, (fpmas::api::model::Model*), (override));

				MOCK_METHOD(fpmas::api::model::AgentTask*, task, (), (override));
				MOCK_METHOD(const fpmas::api::model::AgentTask*, task, (), (const, override));
				MOCK_METHOD(void, setTask, (fpmas::api::model::AgentTask*), (override));
				MOCK_METHOD(fpmas::api::model::AgentTask*, task,
						(fpmas::api::model::GroupId), (override));
				MOCK_METHOD(const fpmas::api::model::AgentTask*, task,
						(fpmas::api::model::GroupId), (const, override));
				MOCK_METHOD(void, setTask,
						(fpmas::api::model::GroupId, fpmas::api::model::AgentTask*), (override));
				MOCK_METHOD((const std::unordered_map<fpmas::api::model::GroupId, fpmas::api::model::AgentTask*>&),
						tasks, (), (override));

				MockAgentBase() {
					ON_CALL(*this, typeId).WillByDefault(Return(TYPE_ID));
					setUpGid();
				}

			private:
				void _addGroupId(fpmas::api::model::GroupId gid) {
					gids.push_back(gid);
				}

				void setUpGid() {
					ON_CALL(*this, groupIds)
						.WillByDefault(ReturnPointee(&gids));
					EXPECT_CALL(*this, groupIds).Times(AnyNumber());
					ON_CALL(*this, addGroupId)
						.WillByDefault(Invoke(this,
									&MockAgentBase<Implem>::_addGroupId));
					EXPECT_CALL(*this, addGroupId).Times(AnyNumber());
				}
		};
	template<typename Implem>
		const fpmas::api::model::TypeId MockAgentBase<Implem>::TYPE_ID = typeid(Implem);
}

/*
 * FOO parameter is just used to easily generate multiple agent types.
 */
template<int FOO = 0>
class MockAgent : public virtual fpmas::api::model::Agent, public detail::MockAgentBase<MockAgent<FOO>> {
	public:
		MOCK_METHOD(void, act, (), (override));

		// A fake custom agent field
		MOCK_METHOD(void, setField, (int), ());
		MOCK_METHOD(int, getField, (), (const));

		MockAgent() : detail::MockAgentBase<MockAgent<FOO>>() {}

		MockAgent(int field) : MockAgent() {
			EXPECT_CALL(*this, getField).Times(AnyNumber())
				.WillRepeatedly(Return(field));
		}
};

class MockModel : public virtual fpmas::api::model::Model {
	protected:
		MOCK_METHOD(void, insert,
				(fpmas::api::model::GroupId, fpmas::api::model::AgentGroup*), (override));
	public:
		MOCK_METHOD(fpmas::api::model::AgentGraph&, graph, (), (override));
		MOCK_METHOD(fpmas::api::scheduler::Scheduler&, scheduler, (), (override));
		MOCK_METHOD(fpmas::api::runtime::Runtime&, runtime, (), (override));
		MOCK_METHOD(const fpmas::api::scheduler::Job&, loadBalancingJob, (), (const, override));

		MOCK_METHOD(fpmas::api::model::AgentGroup&, buildGroup, (fpmas::api::model::GroupId), (override));
		MOCK_METHOD(fpmas::api::model::AgentGroup&, buildGroup, (fpmas::api::model::GroupId, const fpmas::api::model::Behavior&), (override));
		MOCK_METHOD(void, removeGroup, (fpmas::api::model::AgentGroup&), (override));
		MOCK_METHOD(fpmas::api::model::AgentGroup&, getGroup, (fpmas::api::model::GroupId), (const, override));

		MOCK_METHOD((const std::unordered_map<fpmas::api::model::GroupId, fpmas::api::model::AgentGroup*>&),
				groups, (), (const, override));

		MOCK_METHOD(
			fpmas::api::model::AgentEdge*, link,
			(fpmas::api::model::Agent*, fpmas::api::model::Agent*, fpmas::api::graph::LayerId), (override));
		MOCK_METHOD(void, unlink, (fpmas::api::model::AgentEdge*), (override));
		MOCK_METHOD(fpmas::api::communication::MpiCommunicator&,
				getMpiCommunicator, (), (override));
		MOCK_METHOD(const fpmas::api::communication::MpiCommunicator&,
				getMpiCommunicator, (), (const, override));
};

class MockAgentGroup : public virtual fpmas::api::model::AgentGroup {
	public:
		MOCK_METHOD(fpmas::api::model::GroupId, groupId, (), (const, override));
		MOCK_METHOD(fpmas::api::model::Behavior&, behavior, (), (override));
		MOCK_METHOD(void, add, (fpmas::api::model::Agent*), (override));
		MOCK_METHOD(void, remove, (fpmas::api::model::Agent*), (override));
		MOCK_METHOD(void, insert, (fpmas::api::model::AgentPtr*), (override));
		MOCK_METHOD(void, erase, (fpmas::api::model::AgentPtr*), (override));
		MOCK_METHOD(void, clear, (), (override));
		MOCK_METHOD(std::vector<fpmas::api::model::Agent*>, agents, (), (const, override));
		MOCK_METHOD(std::vector<fpmas::api::model::Agent*>, localAgents, (), (const, override));
		MOCK_METHOD(fpmas::api::scheduler::Job&, job, (), (override));
		MOCK_METHOD(const fpmas::api::scheduler::Job&, job, (), (const, override));
		MOCK_METHOD(const fpmas::api::scheduler::Job&, agentExecutionJob, (), (const, override));
		MOCK_METHOD(fpmas::api::scheduler::Job&, agentExecutionJob, (), (override));
		MOCK_METHOD(fpmas::api::scheduler::JobList, jobs, (), (const, override));
};

/*
 * FOO parameter is just used to easily generate multiple agent types.
 */
template<typename AgentType>
class MockAgentBase : public fpmas::model::AgentBase<AgentType> {
	public:
		MockAgentBase():fpmas::model::AgentBase<AgentType>() {
			EXPECT_CALL(*this, _copyAssign).Times(AnyNumber());
			EXPECT_CALL(*this, _moveAssign).Times(AnyNumber());
		}
		MockAgentBase(const MockAgentBase& other)
			: fpmas::model::AgentBase<AgentType>(other) {
				EXPECT_CALL(*this, _copyAssign).Times(AnyNumber());
				EXPECT_CALL(*this, _moveAssign).Times(AnyNumber());
			}

		MockAgentBase& operator=(const MockAgentBase& other) {
			this->_copyAssign(&other);
			return *this;
		}
		MOCK_METHOD(void, _copyAssign, (const MockAgentBase*), ());

		MockAgentBase& operator=(MockAgentBase&& other) {
			this->_moveAssign(&other);
			return *this;
		}
		MOCK_METHOD(void, _moveAssign, (MockAgentBase*), ());

		MOCK_METHOD(void, act, (), (override));
};

template<int FOO = 0>
class DefaultMockAgentBase : public MockAgentBase<DefaultMockAgentBase<FOO>> {};

using MockAgentNode = MockDistributedNode<fpmas::model::AgentPtr>;
using MockAgentEdge = MockDistributedEdge<fpmas::model::AgentPtr>;

template<
	template<typename> class DistNode = MockDistributedNode,
	template<typename> class DistEdge = MockDistributedEdge>
using MockAgentGraph = MockDistributedGraph<
	fpmas::model::AgentPtr,
	DistNode<fpmas::model::AgentPtr>,
	DistEdge<fpmas::model::AgentPtr>>;

namespace nlohmann {
	template<int FOO>
	using MockAgentPtr = fpmas::api::utils::PtrWrapper<MockAgent<FOO>>;

	template<int FOO>
		struct adl_serializer<MockAgentPtr<FOO>> {
			static void to_json(json& j, const MockAgentPtr<FOO>& data) {
				j["mock"] = data->getField();
			}

			static void from_json(const json& j, MockAgentPtr<FOO>& ptr) {
				ptr = MockAgentPtr<FOO>(new MockAgent<FOO>(j.at("field").get<int>()));
			}
		};
}

#endif
