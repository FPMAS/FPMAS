#include "graph/parallel/synchro/hard/hard_sync_mode.h"

#include <random>

#include "graph/parallel/distributed_graph.h"
#include "../mocks/graph/parallel/mock_load_balancing.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::DistributedNode;
using FPMAS::graph::parallel::DistributedArc;
using FPMAS::graph::parallel::DefaultMpiSetUp;
using FPMAS::graph::parallel::LocationManager;
using FPMAS::graph::parallel::synchro::hard::HardSyncMode;
using FPMAS::api::graph::parallel::LocationState;
using FPMAS::communication::TypedMpi;
using FPMAS::communication::MpiCommunicator;
using FPMAS::graph::parallel::synchro::hard::HardSyncMutex;
using FPMAS::graph::parallel::synchro::hard::MutexClient;
using FPMAS::graph::parallel::synchro::hard::MutexServer;
using FPMAS::graph::parallel::synchro::hard::TerminationAlgorithm;
using FPMAS::graph::parallel::synchro::hard::DataUpdatePack;

class MPI_HardSyncMutexSelfReadTest : public ::testing::Test {
	protected:
		MpiCommunicator comm;
		TerminationAlgorithm<TypedMpi> termination {comm};
		TypedMpi<DistributedId> id_mpi {comm};
		TypedMpi<int> data_mpi {comm};
		TypedMpi<DataUpdatePack<int>> data_update_mpi {comm};

		int data = comm.getRank();
		LocationState state = LocationState::DISTANT;
		int location = comm.getRank();

		MutexServer<int> server {comm, id_mpi, data_mpi, data_update_mpi};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server};
		HardSyncMutex<int> mutex {data};

		void SetUp() override {
			mutex.setUp(DistributedId(3, comm.getRank()), state, location, client, server);
			server.manage(DistributedId(3, comm.getRank()), &mutex);

			state = LocationState::LOCAL;
		}
};

/*
 * Each proc read a local unlocked data : no communication needs to occur.
 */
TEST_F(MPI_HardSyncMutexSelfReadTest, unlocked_read_test) {
	int read_data = mutex.read();

	ASSERT_EQ(read_data, comm.getRank());
}

/*
 * mpi_race_condition
 */
class Mpi_MutexServerRaceCondition : public ::testing::Test {
	protected:
		static const int NUM_ACQUIRE = 500;
		MpiCommunicator comm;
		TerminationAlgorithm<TypedMpi> termination {comm};
		TypedMpi<DistributedId> id_mpi {comm};
		TypedMpi<int> data_mpi {comm};
		TypedMpi<DataUpdatePack<int>> data_update_mpi {comm};

		int data = 0;
		LocationState state = LocationState::DISTANT;
		int location = 0;

		MutexServer<int> server {comm, id_mpi, data_mpi, data_update_mpi};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server};
		HardSyncMutex<int> mutex {data};

		void SetUp() override {
			mutex.setUp(DistributedId(3, 6), state, location, client, server);
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
		mutex.releaseAcquire();
	}
	termination.terminate(server);

	if(comm.getRank() == 0) {
		ASSERT_EQ(data, comm.getSize() * NUM_ACQUIRE);
	}
}

class HardSyncModeIntegrationTest : public ::testing::Test {
	protected:
		DistributedGraph<
			int, HardSyncMode,
			DistributedNode,
			DistributedArc,
			DefaultMpiSetUp,
			LocationManager
			> graph;

		std::mt19937 engine;
		std::uniform_int_distribution<int> dist {0, graph.getMpiCommunicator().getSize()-1};
		
		typename decltype(graph)::PartitionMap partition;
};

TEST_F(HardSyncModeIntegrationTest, acquire) {
	// TODO

}
