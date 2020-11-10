#include "gtest/gtest.h"
#include "fpmas.h"
#include "gtest_environment.h"

int main(int argc, char **argv) {
	::testing::AddGlobalTestEnvironment(new Environment);
	std::cout << "Running local test suit of FPMAS " << FPMAS_VERSION << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
