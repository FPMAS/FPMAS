#ifndef FPMAS_GRID_MOCK_H
#define FPMAS_GRID_MOCK_H

#include "fpmas/api/model/spatial/grid.h"
#include "mock_spatial_model.h"

class AbstractMockGridCell : public fpmas::api::model::GridCell {
	public:
		MOCK_METHOD(fpmas::api::model::DiscretePoint, location, (), (const, override));
};

template<template<typename> class Strictness>
class MockGridCell : public Strictness<AbstractMockGridCell>, public MockCell<Strictness> {
};

class MockGridCellFactory : public fpmas::api::model::GridCellFactory {
	public:
		MOCK_METHOD(fpmas::api::model::GridCell*, build,
				(fpmas::api::model::DiscretePoint), (override));

};
#endif
