#ifndef MOCK_MODEL_H
#define MOCK_MODEL_H
#include "gmock/gmock.h"
#include "api/model/model.h"

class MockAgent : public FPMAS::api::model::Agent {
	public:
		MOCK_METHOD(FPMAS::api::model::AgentNode*, node, (), (override));
		MOCK_METHOD(const FPMAS::api::model::AgentNode*, node, (), (const, override));
		MOCK_METHOD(void, setNode, (FPMAS::api::model::AgentNode*), (override));
		MOCK_METHOD(void, act, (), (override));

};
#endif
