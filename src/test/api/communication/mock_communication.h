#ifndef MPI_COMMUNICATOR_MOCK_H
#define MPI_COMMUNICATOR_MOCK_H

#include "gmock/gmock.h"

#include "api/communication/resource_handler.h"
#include "api/communication/communication.h"

using ::testing::Return;
using ::testing::AnyNumber;

using FPMAS::api::communication::Color;
using FPMAS::api::communication::Tag;
using FPMAS::api::communication::Epoch;

class MockResourceHandler : public FPMAS::api::communication::ResourceHandler {
	public:
		MOCK_METHOD(std::string, getLocalData, (DistributedId), (const, override));
		MOCK_METHOD(std::string, getUpdatedData, (DistributedId), (const, override));
		MOCK_METHOD(void, updateData, (DistributedId, std::string), (override));

};

template<int RANK=0, int SIZE=0>
class MockMpiCommunicator : public FPMAS::api::communication::MpiCommunicator {
	public:
		MockMpiCommunicator() {
			ON_CALL(*this, getRank())
				.WillByDefault(Return(RANK));
			EXPECT_CALL(*this, getRank())
				.Times(AnyNumber());
			ON_CALL(*this, getSize())
				.WillByDefault(Return(SIZE));
			EXPECT_CALL(*this, getSize())
				.Times(AnyNumber());
		}

		MOCK_METHOD(int, getRank, (), (const, override));
		MOCK_METHOD(int, getSize, (), (const, override));

		MOCK_METHOD(void, send, (Color, int), (override));
		MOCK_METHOD(void, sendEnd, (int), (override));
		MOCK_METHOD(void, Issend, (const DistributedId&, int, int, MPI_Request*), (override));
		MOCK_METHOD(void, Issend, (const std::string&, int, int, MPI_Request*), (override));
		MOCK_METHOD(void, Isend, (const std::string&, int, int, MPI_Request*), (override));

		MOCK_METHOD(void, probe, (int, int, MPI_Status*), (override));
		MOCK_METHOD(int, Iprobe, (int, int, MPI_Status*), (override));

		MOCK_METHOD(void, recvEnd, (MPI_Status*), (override));
		MOCK_METHOD(void, recv, (MPI_Status*, Color&), (override));
		MOCK_METHOD(void, recv, (MPI_Status*, std::string&), (override));
		MOCK_METHOD(void, recv, (MPI_Status*, DistributedId&), (override));

		MOCK_METHOD((std::unordered_map<int, std::string>), allToAll, ((std::unordered_map<int, std::string>)), (override));
};

template<typename T>
class MockMpi : public FPMAS::api::communication::Mpi<T> {
	public:
		MockMpi(FPMAS::api::communication::MpiCommunicator&) {
			EXPECT_CALL(*this, migrate).Times(AnyNumber());
		}

		MOCK_METHOD(
			(std::unordered_map<int, std::vector<T>>), 
			migrate,
			((std::unordered_map<int, std::vector<T>>)),
			(override)
			);

};

template<int RANK, int SIZE>
using MockMpiSetUp = FPMAS::api::communication::MpiSetUp<MockMpiCommunicator<RANK, SIZE>, MockMpi>;

#endif
