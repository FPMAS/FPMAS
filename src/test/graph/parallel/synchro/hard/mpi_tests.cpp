#include "gtest/gtest.h"

#include "graph/parallel/synchro/hard/hard_sync_mutex.h"
#include "graph/parallel/synchro/hard/mutex_client.h"
#include "graph/parallel/synchro/hard/mutex_server.h"
#include "graph/parallel/synchro/hard/termination.h"
#include "communication/communication.h"

using FPMAS::api::graph::parallel::LocationState;
using FPMAS::communication::Mpi;
using FPMAS::communication::MpiCommunicator;
using FPMAS::graph::parallel::synchro::hard::HardSyncMutex;
using FPMAS::graph::parallel::synchro::hard::MutexClient;
using FPMAS::graph::parallel::synchro::hard::MutexServer;
using FPMAS::graph::parallel::synchro::hard::TerminationAlgorithm;

/*
 * mpi_race_condition
 */
class Mpi_MutexServerRaceCondition : public ::testing::Test {
	protected:
		static const int NUM_ACQUIRE = 10;
	MpiCommunicator comm;
	TerminationAlgorithm<int> termination {comm};

	int data = 0;
	LocationState state = LocationState::DISTANT;
	int location = 0;

	MutexServer<int, Mpi> server {comm};
	MutexClient<int, Mpi> client {comm, server};
	HardSyncMutex<int> mutex {DistributedId(3, 6), data, state, location, client, server};


	void SetUp() override {
		client.manage(DistributedId(3, 6), &mutex);
		server.manage(DistributedId(3, 6), &mutex);
		if(comm.getRank() == 0) {
			state = LocationState::LOCAL;
		}
	}
};

TEST_F(Mpi_MutexServerRaceCondition, acquire_race_condition) {
	for(int i = 0; i < NUM_ACQUIRE; i++) {
		int& data = mutex.acquire();
		data++;
		mutex.release();
	}
	for(int i = 0; i < 10000; i++) {
		server.handleIncomingRequests();
	}
	//termination.terminate(server);

	if(comm.getRank() == 0) {
		ASSERT_EQ(data, comm.getSize() * NUM_ACQUIRE);
	}
}
