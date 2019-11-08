#include "gtest/gtest.h"
#include "communication/communication.h"
#include "mpi.h"
TEST(CommunicationTest, size_test) {
	Communication communication;

	ASSERT_EQ(MPI::COMM_WORLD.Get_size(), communication.size());
}
TEST(CommunicationTest, rank_test) {
	Communication communication;

	ASSERT_EQ(MPI::COMM_WORLD.Get_rank(), communication.rank());
}
