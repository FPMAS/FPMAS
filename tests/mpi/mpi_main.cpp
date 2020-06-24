#include "gtest/gtest.h"
#include "mpi.h"
#include "zoltan_cpp.h"

#include "fpmas/communication/communication.h"

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);

	MPI_Init(&argc, &argv);
	float v;
	Zoltan_Initialize(argc, argv, &v);
	fpmas::communication::createMpiTypes();

	int result;
	result =  RUN_ALL_TESTS();

	fpmas::communication::freeMpiTypes();
	MPI_Finalize();
	return result;
}
