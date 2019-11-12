#include "gtest/gtest.h"
#include "communication/communication.h"

TEST(CommunicationTest, size_test) {
	Communication communication;

	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	ASSERT_EQ(size, communication.size());
}
TEST(CommunicationTest, rank_test) {
	Communication communication;

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	ASSERT_EQ(rank, communication.rank());
}
