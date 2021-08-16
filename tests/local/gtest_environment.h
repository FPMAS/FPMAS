#ifndef FPMAS_GTEST_LOCAL_ENVIRONMENT_H
#define FPMAS_GTEST_LOCAL_ENVIRONMENT_H

#include "model/mock_model.h"
#include "model/mock_spatial_model.h"
#include "fpmas/model/spatial/grid.h"
#include "../mocks/model/mock_grid.h"

namespace model { namespace test {
	template<typename AgentType>
		class SpatialAgentBase : public fpmas::model::SpatialAgent<AgentType, fpmas::api::model::Cell> {
			public:
				testing::NiceMock<MockRange<fpmas::api::model::Cell>> mobility_range;
				testing::NiceMock<MockRange<fpmas::api::model::Cell>> perception_range;

				SpatialAgentBase() {}
				SpatialAgentBase(const SpatialAgentBase&)
					: SpatialAgentBase() {}
				SpatialAgentBase(SpatialAgentBase&&)
					: SpatialAgentBase() {}
				SpatialAgentBase& operator=(const SpatialAgentBase&) {return *this;}
				SpatialAgentBase& operator=(SpatialAgentBase&&) {return *this;}

				FPMAS_MOBILITY_RANGE(mobility_range);
				FPMAS_PERCEPTION_RANGE(perception_range);
		};

	class SpatialAgent : public SpatialAgentBase<SpatialAgent> {};

	class SpatialAgentWithData : public SpatialAgentBase<SpatialAgentWithData> {
		public:
			int data;

			SpatialAgentWithData(int data) : data(data) {}

			static void to_json(nlohmann::json& j, const SpatialAgentWithData* agent) {
				j = agent->data;
			}
			static SpatialAgentWithData* from_json(const nlohmann::json& j) {
				auto agent = new SpatialAgentWithData(j.get<int>());;
				return agent;
			}
	};

	template<typename GridAgentType>
		class GridAgentBase : public fpmas::model::GridAgent<GridAgentType, MockGridCell<testing::NiceMock>> {
			public:
				static MockRange<MockGridCell<testing::NiceMock>>* mobility_range;
				static MockRange<MockGridCell<testing::NiceMock>>* perception_range;

				FPMAS_MOBILITY_RANGE(*mobility_range);
				FPMAS_PERCEPTION_RANGE(*perception_range);
		};

	template<typename GridAgentType>
	MockRange<MockGridCell<testing::NiceMock>>* GridAgentBase<GridAgentType>::mobility_range;
	template<typename GridAgentType>
	MockRange<MockGridCell<testing::NiceMock>>* GridAgentBase<GridAgentType>::perception_range;

	class GridAgent : public GridAgentBase<GridAgent> {};

	class GridAgentWithData : public GridAgentBase<GridAgentWithData> {
		public:
			int data;

			GridAgentWithData() : data(0) {}
			GridAgentWithData(int data)
				: data(data) {}

			static GridAgentWithData* from_json(const nlohmann::json& j) {
				return new GridAgentWithData(j.get<int>());
			}
			static void to_json(nlohmann::json& j, const GridAgentWithData* agent) {
				j = agent->data;
			}
	};
}}

FPMAS_DEFAULT_JSON(model::test::SpatialAgent)
FPMAS_DEFAULT_JSON(model::test::GridAgent)

class Environment : public testing::Environment {
	void SetUp() override {
		FPMAS_REGISTER_AGENT_TYPES(
		MockAgent<4>, MockAgent<2>, MockAgent<12>,
		model::test::SpatialAgent::JsonBase,
		model::test::SpatialAgentWithData::JsonBase,
		model::test::GridAgent::JsonBase,
		model::test::GridAgentWithData::JsonBase,
		fpmas::model::GridCell::JsonBase
		);
	}

	void TearDown() override {

	}
};

#endif
