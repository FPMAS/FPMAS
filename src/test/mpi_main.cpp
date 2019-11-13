#include "gtest/gtest.h"
#include "mpi.h"

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	MPI::Init(argc, argv);
	int result;
	// if(MPI::COMM_WORLD.Get_rank() == 0)
		result =  RUN_ALL_TESTS();
	MPI::Finalize();
	return result;
}
