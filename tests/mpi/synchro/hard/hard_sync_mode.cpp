#include "fpmas/synchro/hard/hard_sync_mode.h"

#include <random>

#include "fpmas/graph/parallel/distributed_graph.h"
#include "../mocks/graph/parallel/mock_distributed_node.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/synchro/hard/mock_client_server.h"

using fpmas::api::graph::parallel::LocationState;
using fpmas::communication::TypedMpi;
using fpmas::communication::MpiCommunicator;
using fpmas::graph::parallel::DistributedGraph;
using fpmas::graph::parallel::DistributedNode;
using fpmas::graph::parallel::DistributedArc;
using fpmas::graph::parallel::DefaultMpiSetUp;
using fpmas::graph::parallel::LocationManager;
using fpmas::synchro::HardSyncMode;
using fpmas::synchro::hard::HardSyncMutex;
using fpmas::synchro::hard::MutexClient;
using fpmas::synchro::hard::MutexServer;
using fpmas::synchro::hard::TerminationAlgorithm;
using fpmas::synchro::hard::DataUpdatePack;

class HardSyncMutexSelfReadTest : public ::testing::Test {
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

		MockLinkServer mock_link_server;
		MutexServer<int> mutex_server {comm, id_mpi, data_mpi, data_update_mpi};
		fpmas::synchro::hard::ServerPack<int> server_pack {comm, termination, mutex_server, mock_link_server};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server_pack};
		HardSyncMutex<int> mutex {&node, client, mutex_server};

		void SetUp() override {
			ON_CALL(node, state).WillByDefault(ReturnPointee(&state));
			ON_CALL(node, getLocation).WillByDefault(ReturnPointee(&location));
			server_pack.mutexServer().manage(DistributedId(3, comm.getRank()), &mutex);

			state = LocationState::LOCAL;
		}
};

/*
 * Each proc read a local unlocked data : no communication needs to occur.
 */
TEST_F(HardSyncMutexSelfReadTest, unlocked_read_test) {
	int read_data = mutex.read();

	ASSERT_EQ(read_data, comm.getRank());
}

/*
 * mpi_race_condition
 */
class MutexServerRaceCondition : public ::testing::Test {
	protected:
		static const int NUM_ACQUIRE = 500;
		MpiCommunicator comm;
		TerminationAlgorithm<TypedMpi> termination {comm};
		TypedMpi<DistributedId> id_mpi {comm};
		TypedMpi<int> data_mpi {comm};
		TypedMpi<DataUpdatePack<int>> data_update_mpi {comm};

		int data = 0;
		MockDistributedNode<int> node {DistributedId(3, 6), data};

		LocationState state = LocationState::DISTANT;
		int location = 0;

		MutexServer<int> mutex_server {comm, id_mpi, data_mpi, data_update_mpi};
		MockLinkServer mock_link_server;
		fpmas::synchro::hard::ServerPack<int> server_pack {comm, termination, mutex_server, mock_link_server};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server_pack};
		HardSyncMutex<int> mutex {&node, client, mutex_server};

		void SetUp() override {
			EXPECT_CALL(mock_link_server, handleIncomingRequests).Times(AnyNumber());
			ON_CALL(node, state).WillByDefault(ReturnPointee(&state));
			ON_CALL(node, getLocation).WillByDefault(ReturnPointee(&location));

			server_pack.mutexServer().manage(DistributedId(3, 6), &mutex);
			if(comm.getRank() == 0) {
				state = LocationState::LOCAL;
			}
		}
};

TEST_F(MutexServerRaceCondition, acquire_race_condition) {
	for(int i = 0; i < NUM_ACQUIRE; i++) {
		int& data = mutex.acquire();
		data++;
		mutex.releaseAcquire();
	}
	termination.terminate(server_pack);

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
