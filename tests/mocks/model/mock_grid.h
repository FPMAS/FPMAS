#ifndef FPMAS_GRID_MOCK_H
#define FPMAS_GRID_MOCK_H

#include "fpmas/api/model/spatial/grid.h"
#include "mock_spatial_model.h"

class MockGridCell : public MockCell<fpmas::api::model::GridCell> {
	public:
		MOCK_METHOD(fpmas::api::model::DiscretePoint, location, (), (const, override));
};

template<typename GridCell>
class MockGridCellFactory : public fpmas::api::model::GridCellFactory<GridCell> {
	public:
		MOCK_METHOD(GridCell*, build,
				(fpmas::api::model::DiscretePoint), (override));

};
#endif
