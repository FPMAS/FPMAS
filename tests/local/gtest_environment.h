#ifndef FPMAS_GTEST_LOCAL_ENVIRONMENT_H
#define FPMAS_GTEST_LOCAL_ENVIRONMENT_H

#include "../mocks/model/mock_model.h"
#include "../mocks/model/mock_environment.h"
#include "fpmas/model/grid.h"

#define MOBILITY_RANGE(RANGE) \
	std::add_lvalue_reference<std::add_const<decltype(RANGE)>::type>::type\
		mobilityRange() const override {\
		return RANGE;\
	}
#define PERCEPTION_RANGE(RANGE) \
	std::add_lvalue_reference<std::add_const<decltype(RANGE)>::type>::type\
		perceptionRange() const override {\
		return RANGE;\
	}

namespace model { namespace test {
	template<typename GridAgentType>
		class GridAgentBase : public fpmas::model::GridAgent<GridAgentType> {
			public:
				static MockRange<fpmas::api::model::GridCell>* mobility_range;
				static MockRange<fpmas::api::model::GridCell>* perception_range;

				MOBILITY_RANGE(*mobility_range);
				PERCEPTION_RANGE(*perception_range);

		};

	template<typename GridAgentType>
	MockRange<fpmas::api::model::GridCell>* GridAgentBase<GridAgentType>::mobility_range;
	template<typename GridAgentType>
	MockRange<fpmas::api::model::GridCell>* GridAgentBase<GridAgentType>::perception_range;

	class GridAgent : public GridAgentBase<GridAgent> {};

	class GridAgentWithData : public GridAgentBase<GridAgentWithData> {
		public:
			int data;

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

FPMAS_DEFAULT_JSON(model::test::GridAgent)

FPMAS_JSON_SET_UP(
		MockAgent<4>, MockAgent<2>, MockAgent<12>,
		model::test::GridAgent::JsonBase,
		model::test::GridAgentWithData::JsonBase,
		fpmas::model::GridCell::JsonBase
		)

class Environment : public testing::Environment {
	void SetUp() override {
		FPMAS_REGISTER_AGENT_TYPES(
		MockAgent<4>, MockAgent<2>, MockAgent<12>,
		model::test::GridAgent::JsonBase,
		model::test::GridAgentWithData::JsonBase,
		fpmas::model::GridCell::JsonBase
		);
	}

	void TearDown() override {

	}
};

#endif
