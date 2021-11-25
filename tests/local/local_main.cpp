#include "gtest/gtest.h"
#include "fpmas.h"

FPMAS_DEFAULT_JSON_SET_UP();

int main(int argc, char **argv) {
	std::cout << "Running local test suit of FPMAS " << FPMAS_VERSION << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
