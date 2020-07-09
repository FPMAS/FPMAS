#ifndef MPI_COMMUNICATOR_MOCK_H
#define MPI_COMMUNICATOR_MOCK_H

#include "gmock/gmock.h"

//#include "fpmas/api/communication/resource_handler.h"
#include "fpmas/api/communication/communication.h"

using ::testing::Return;
using ::testing::AnyNumber;

template<int RANK=0, int SIZE=0>
class MockMpiCommunicator : public fpmas::api::communication::MpiCommunicator {
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

		MOCK_METHOD(void, send, (const void*, int, MPI_Datatype, int, int), (override));
		MOCK_METHOD(void, send, (int, int), (override));
		MOCK_METHOD(void, Issend, (const void*, int, MPI_Datatype, int, int, MPI_Request*), (override));
		MOCK_METHOD(void, Issend, (int, int, MPI_Request*), (override));

		MOCK_METHOD(void, recv, (int, int, MPI_Status*), (override));
		MOCK_METHOD(void, recv, (void* buffer, int count, MPI_Datatype, int, int, MPI_Status*), (override));

		MOCK_METHOD(void, probe, (int, int, MPI_Status*), (override));
		MOCK_METHOD(bool, Iprobe, (int, int, MPI_Status*), (override));
		MOCK_METHOD(bool, test, (MPI_Request*), (override));

		MOCK_METHOD(
			(std::unordered_map<int, fpmas::api::communication::DataPack>), allToAll,
			((std::unordered_map<int, fpmas::api::communication::DataPack>), MPI_Datatype), (override));
		MOCK_METHOD(std::vector<fpmas::api::communication::DataPack>, gather, 
				(fpmas::api::communication::DataPack, MPI_Datatype, int), (override));
};

template<typename T>
class MockMpi : public fpmas::api::communication::TypedMpi<T> {
	public:
		MockMpi(fpmas::api::communication::MpiCommunicator&) {
			EXPECT_CALL(*this, migrate).Times(AnyNumber());
		}

		MOCK_METHOD(
			(std::unordered_map<int, std::vector<T>>), 
			migrate,
			((std::unordered_map<int, std::vector<T>>)),
			(override)
			);
		MOCK_METHOD(std::vector<T>, gather, (const T&, int), (override));
		MOCK_METHOD(void, send, (const T&, int, int), (override));
		MOCK_METHOD(void, Issend, (const T&, int, int, MPI_Request*), (override));
		MOCK_METHOD(T, recv, (int, int, MPI_Status*), (override));

};

template<int RANK, int SIZE>
using MockMpiSetUp = fpmas::api::communication::MpiSetUp<MockMpiCommunicator<RANK, SIZE>, MockMpi>;

#endif
