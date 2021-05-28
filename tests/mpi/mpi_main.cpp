#include "gtest/gtest.h"

#include "fpmas.h"
#include "model/test_agents.h"

int main(int argc, char **argv) {
	std::cout << "Running MPI test suit of FPMAS " << FPMAS_VERSION << std::endl;
	::testing::InitGoogleTest(&argc, argv);
	FPMAS_REGISTER_AGENT_TYPES(TEST_AGENTS)

	fpmas::init(argc, argv);

	int result;
	result =  RUN_ALL_TESTS();

	fpmas::finalize();
	return result;
}
