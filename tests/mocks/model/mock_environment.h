#ifndef MOCK_ENVIRONMENT_H
#define MOCK_ENVIRONMENT_H

#include "gmock/gmock.h"
#include "fpmas/model/environment.h"
#include "mock_model.h"

class MockLocatedAgent : public fpmas::api::model::LocatedAgent, public testing::NiceMock<detail::MockAgentBase<MockLocatedAgent>> {
	public:
		MOCK_METHOD(void, act, (), (override));
		MOCK_METHOD(void, moveToCell, (fpmas::api::model::Cell*), (override));
		MOCK_METHOD(fpmas::api::model::Cell*, location, (), (override));
		MOCK_METHOD(unsigned int, mobilityRange, (), (override));
		MOCK_METHOD(bool, isInMobilityRange, (fpmas::api::model::Cell*), (override));
		MOCK_METHOD(unsigned int, perceptionRange, (), (override));
		MOCK_METHOD(bool, isInPerceptionRange, (fpmas::api::model::Cell*), (override));
};

class MockCellBase : public fpmas::model::CellBase, public testing::NiceMock<detail::MockAgentBase<MockCellBase>> {

};

class MockCell : public fpmas::api::model::Cell, public detail::MockAgentBase<MockCell> {

};

#endif
