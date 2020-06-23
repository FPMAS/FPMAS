#include "synchro/hard/hard_sync_mode.h"

#include <random>

#include "graph/parallel/distributed_graph.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/load_balancing/mock_load_balancing.h"

using FPMAS::api::graph::parallel::LocationState;
using FPMAS::communication::TypedMpi;
using FPMAS::communication::MpiCommunicator;
using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::parallel::DistributedNode;
using FPMAS::graph::parallel::DistributedArc;
using FPMAS::graph::parallel::DefaultMpiSetUp;
using FPMAS::graph::parallel::LocationManager;
using FPMAS::synchro::HardSyncMode;
using FPMAS::synchro::hard::HardSyncMutex;
using FPMAS::synchro::hard::MutexClient;
using FPMAS::synchro::hard::MutexServer;
using FPMAS::synchro::hard::TerminationAlgorithm;
using FPMAS::synchro::hard::DataUpdatePack;

class MPI_HardSyncMutexSelfReadTest : public ::testing::Test {
	protected:
		MpiCommunicator comm;
		TerminationAlgorithm<TypedMpi> termination {comm};
		TypedMpi<DistributedId> id_mpi {comm};
		TypedMpi<int> data_mpi {comm};
		TypedMpi<DataUpdatePack<int>> data_update_mpi {comm};

		int data = comm.getRank();
		MockDistributedNode<int> node {DistributedId(3, comm.getRank()), data};

		LocationState state = LocationState::DISTANT;
		int location = comm.getRank();

		MutexServer<int> server {comm, id_mpi, data_mpi, data_update_mpi};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server};
		HardSyncMutex<int> mutex {&node, client, server};

		void SetUp() override {
			ON_CALL(node, state).WillByDefault(ReturnPointee(&state));
			ON_CALL(node, getLocation).WillByDefault(ReturnPointee(&location));
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
		//MockDistributedNode<int> node {DistributedId(3, comm.getRank()), data};
		MockDistributedNode<int> node {DistributedId(3, 6), data};

		LocationState state = LocationState::DISTANT;
		int location = 0;

		MutexServer<int> server {comm, id_mpi, data_mpi, data_update_mpi};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server};
		HardSyncMutex<int> mutex {&node, client, server};

		void SetUp() override {
			ON_CALL(node, state).WillByDefault(ReturnPointee(&state));
			ON_CALL(node, getLocation).WillByDefault(ReturnPointee(&location));

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
		ASSERT_EQ(mutex.read(), comm.getSize() * NUM_ACQUIRE);
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
