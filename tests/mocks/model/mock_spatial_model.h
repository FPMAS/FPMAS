#ifndef MOCK_ENVIRONMENT_H
#define MOCK_ENVIRONMENT_H

#include "gmock/gmock.h"
#include "fpmas/model/spatial/spatial_model.h"
#include "mock_model.h"

template<typename CellType>
class MockRange : public fpmas::api::model::Range<CellType> {
	public:
		MOCK_METHOD(bool, contains, (CellType*, CellType*), (const, override));
		MOCK_METHOD(std::size_t, radius, (CellType*), (const, override));

		virtual ~MockRange() {}
};

template<typename CellType>
class AbstractMockSpatialAgent : public fpmas::api::model::SpatialAgent<CellType> {
	public:
		MOCK_METHOD(void, act, (), (override));
		MOCK_METHOD(void, moveTo, (CellType*), (override));
		MOCK_METHOD(void, moveTo, (fpmas::api::graph::DistributedId), (override));
		MOCK_METHOD(void, initLocation, (CellType*), (override));
		MOCK_METHOD(CellType*, locationCell, (), (const, override));
		MOCK_METHOD(fpmas::api::graph::DistributedId, locationId, (), (const, override));
		MOCK_METHOD(void, handleNewMove, (), (override));
		MOCK_METHOD(void, handleNewPerceive, (), (override));
		MOCK_METHOD(const fpmas::api::model::Range<CellType>&, mobilityRange, (), (const, override));
		MOCK_METHOD(const fpmas::api::model::Range<CellType>&, perceptionRange, (), (const, override));
};
template<typename CellType, template<typename> class Strictness = testing::NaggyMock>
class MockSpatialAgent :
	public Strictness<AbstractMockSpatialAgent<CellType>>,
	public Strictness<detail::MockAgentBase<MockSpatialAgent<CellType, Strictness>>> {
};

class MockCellBase : public fpmas::model::CellBase, public testing::NiceMock<detail::MockAgentBase<MockCellBase>> {
	public:
		MOCK_METHOD(void, act, (), (override));

		virtual ~MockCellBase() {}
};

class AbstractMockCell : public virtual fpmas::api::model::Cell {
	public:
		MOCK_METHOD(void, act, (), (override));

		MOCK_METHOD(std::vector<fpmas::api::model::Cell*>, successors,
				(), (override));
		MOCK_METHOD(void, handleNewLocation, (), (override));
		MOCK_METHOD(void, handleMove, (), (override));
		MOCK_METHOD(void, handlePerceive, (), (override));
		MOCK_METHOD(void, updatePerceptions, (fpmas::api::model::AgentGroup&), (override));

		virtual ~AbstractMockCell() {}
};

template<template<typename> class Strictness = testing::NaggyMock>
class MockCell : public Strictness<AbstractMockCell>, public Strictness<detail::MockAgentBase<MockCell<Strictness>>> {
};

template<typename CellType>
class MockDistributedMoveAlgorithm : public fpmas::api::model::DistributedMoveAlgorithm<CellType> {
	public:
		MOCK_METHOD(fpmas::api::scheduler::JobList, jobs, (), (const, override));
};

template<typename CellType>
class MockMoveAgentGroup :
	public fpmas::api::model::MoveAgentGroup<CellType>,
	public MockAgentGroup {
	public:
		MOCK_METHOD(fpmas::api::model::DistributedMoveAlgorithm<CellType>&,
				distributedMoveAlgorithm, (), (override));
};

template<typename CellType>
class AbstractMockSpatialModel : public fpmas::api::model::SpatialModel<CellType> {
	public:
		MOCK_METHOD(void, add, (CellType*), (override));
		MOCK_METHOD(std::vector<CellType*>, cells, (), (override));
		MOCK_METHOD(fpmas::api::model::MoveAgentGroup<CellType>&, buildMoveGroup,
				(fpmas::model::GroupId, const fpmas::api::model::Behavior&), (override));
};

template<typename CellType, template<typename> class Strictness = testing::NaggyMock>
class MockSpatialModel :
	public Strictness<AbstractMockSpatialModel<CellType>>,
	public Strictness<MockModel> {
};

template<typename CellType>
class MockSpatialAgentFactory : public fpmas::api::model::SpatialAgentFactory<CellType> {
	public:
		MOCK_METHOD(fpmas::api::model::SpatialAgent<CellType>*, build, (), (override));
};

template<typename CellType>
class MockSpatialAgentMapping : public fpmas::api::model::SpatialAgentMapping<CellType> {
	public:
		MOCK_METHOD(std::size_t, countAt, (CellType*), (override));
};

#endif
