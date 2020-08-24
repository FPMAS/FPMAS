#include "gtest/gtest.h"

#include "fpmas.h"

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);

	fpmas::init(argc, argv);

	int result;
	result =  RUN_ALL_TESTS();

	fpmas::finalize();
	return result;
}
