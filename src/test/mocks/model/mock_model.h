#ifndef MOCK_MODEL_H
#define MOCK_MODEL_H
#include "gmock/gmock.h"
#include "api/model/model.h"

class MockAgent : public FPMAS::api::model::Agent {
	public:
		MOCK_METHOD(FPMAS::api::model::GroupId, groupId, (), (const, override));
		MOCK_METHOD(FPMAS::api::model::AgentNode*, node, (), (override));
		MOCK_METHOD(const FPMAS::api::model::AgentNode*, node, (), (const, override));
		MOCK_METHOD(void, setNode, (FPMAS::api::model::AgentNode*), (override));

		MOCK_METHOD(FPMAS::api::model::AgentTask*, task, (), (override));
		MOCK_METHOD(const FPMAS::api::model::AgentTask*, task, (), (const, override));
		MOCK_METHOD(void, setTask, (FPMAS::api::model::AgentTask*), (override));

		MOCK_METHOD(void, act, (), (override));

};

class MockModel : public FPMAS::api::model::Model {
	public:
		MOCK_METHOD(AgentGraph&, graph, (), (override));
		MOCK_METHOD(FPMAS::api::scheduler::Scheduler&, scheduler, (), (override));
		MOCK_METHOD(FPMAS::api::runtime::Runtime&, runtime, (), (override));
		MOCK_METHOD(const FPMAS::api::scheduler::Job&, loadBalancingJob, (), (const, override));

		MOCK_METHOD(FPMAS::api::model::AgentGroup&, buildGroup, (), (override));
		MOCK_METHOD(FPMAS::api::model::AgentGroup&, getGroup, (FPMAS::api::model::GroupId), (override));

		MOCK_METHOD((const std::unordered_map<FPMAS::api::model::GroupId, FPMAS::api::model::AgentGroup*>&),
				groups, (), (const, override));
};
#endif
