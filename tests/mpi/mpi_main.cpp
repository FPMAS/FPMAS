#include "gtest/gtest.h"

#include "fpmas.h"

int main(int argc, char **argv) {
	std::cout << "Running MPI test suit of FPMAS " << FPMAS_VERSION << std::endl;
	::testing::InitGoogleTest(&argc, argv);

	fpmas::init(argc, argv);

	int result;
	result =  RUN_ALL_TESTS();

	fpmas::finalize();
	return result;
}
