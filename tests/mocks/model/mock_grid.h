#ifndef FPMAS_GRID_MOCK_H
#define FPMAS_GRID_MOCK_H

#include "fpmas/api/model/grid.h"
#include "mock_spatial_model.h"

class MockGridCell : public fpmas::api::model::GridCell, public testing::NiceMock<MockCell> {
	public:
		MOCK_METHOD(fpmas::api::model::DiscretePoint, location, (), (const, override));

		virtual ~MockGridCell() {}
};

class MockGridCellFactory : public fpmas::api::model::GridCellFactory {
	public:
		MOCK_METHOD(fpmas::api::model::GridCell*, build,
				(fpmas::api::model::DiscretePoint), (override));

};
#endif
