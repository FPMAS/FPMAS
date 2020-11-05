#ifndef MOCK_ENVIRONMENT_H
#define MOCK_ENVIRONMENT_H

#include "gmock/gmock.h"
#include "fpmas/model/environment.h"
#include "mock_model.h"

class MockRange : public fpmas::api::model::Range {
	public:
		MOCK_METHOD(unsigned int, size, (), (const, override));
		MOCK_METHOD(bool, contains, (fpmas::api::model::Cell*), (const, override));
};

class MockLocatedAgent : public fpmas::api::model::LocatedAgent, public testing::NiceMock<detail::MockAgentBase<MockLocatedAgent>> {
	public:
		MOCK_METHOD(void, act, (), (override));
		MOCK_METHOD(void, moveToCell, (fpmas::api::model::Cell*), (override));
		MOCK_METHOD(fpmas::api::model::Cell*, location, (), (override));
		MOCK_METHOD(const fpmas::api::model::Range&, mobilityRange, (), (const, override));
		MOCK_METHOD(const fpmas::api::model::Range&, perceptionRange, (), (const, override));
};

class MockCellBase : public fpmas::model::CellBase, public testing::NiceMock<detail::MockAgentBase<MockCellBase>> {
	public:
		MOCK_METHOD(void, act, (), (override));
};

class MockCell : public fpmas::api::model::Cell, public detail::MockAgentBase<MockCell> {

};

#endif
