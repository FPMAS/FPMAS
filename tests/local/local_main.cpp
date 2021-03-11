#include "gtest/gtest.h"
#include "fpmas.h"
#include "gtest_environment.h"

FPMAS_JSON_SET_UP(
		MockAgent<4>, MockAgent<2>, MockAgent<12>,
		::model::test::SpatialAgent::JsonBase,
		::model::test::SpatialAgentWithData::JsonBase,
		::model::test::GridAgent::JsonBase,
		::model::test::GridAgentWithData::JsonBase,
		fpmas::model::GridCell::JsonBase
		)

int main(int argc, char **argv) {
	::testing::AddGlobalTestEnvironment(new Environment);
	std::cout << "Running local test suit of FPMAS " << FPMAS_VERSION << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
