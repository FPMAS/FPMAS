#ifndef FPMAS_GRID_MOCK_H
#define FPMAS_GRID_MOCK_H

#include "fpmas/api/model/grid.h"
#include "mock_environment.h"

class MockGridCell : public fpmas::api::model::GridCell, public testing::NiceMock<MockCell> {
	public:
		MOCK_METHOD(fpmas::api::model::DiscretePoint, location, (), (const, override));

		virtual ~MockGridCell() {}
};

#endif
