#include "fpmas/synchro/hard/hard_sync_mode.h"

#include <random>

#include "fpmas/graph/distributed_graph.h"
#include "../mocks/graph/mock_distributed_node.h"
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/synchro/hard/mock_client_server.h"

#include "utils/test.h"

using ::testing::SizeIs;
using ::testing::Ge;
using ::testing::IsEmpty;

using fpmas::api::graph::LocationState;
using fpmas::communication::TypedMpi;
using fpmas::communication::MpiCommunicator;
using fpmas::graph::DistributedGraph;
using fpmas::graph::DistributedNode;
using fpmas::graph::DistributedEdge;
using fpmas::graph::LocationManager;
using fpmas::synchro::HardSyncMode;
using fpmas::synchro::hard::api::Color;
using fpmas::synchro::hard::HardSyncMutex;
using fpmas::synchro::hard::MutexClient;
using fpmas::synchro::hard::MutexServer;
using fpmas::synchro::hard::TerminationAlgorithm;
using fpmas::synchro::DataUpdatePack;

class HardSyncMutexSelfReadTest : public ::testing::Test {
	protected:
		MpiCommunicator comm;
		TypedMpi<Color> color_mpi {comm};
		TerminationAlgorithm termination {comm, color_mpi};
		TypedMpi<DistributedId> id_mpi {comm};
		TypedMpi<int> data_mpi {comm};
		TypedMpi<DataUpdatePack<int>> data_update_mpi {comm};

		int data = comm.getRank();
		MockDistributedNode<int> node {DistributedId(3, comm.getRank()), data};

		LocationState state = LocationState::DISTANT;
		int location = comm.getRank();

		MockLinkServer mock_link_server;
		MutexServer<int> mutex_server {comm, id_mpi, data_mpi, data_update_mpi, mock_link_server};
		fpmas::synchro::hard::ServerPack<int> server_pack {comm, termination, mutex_server, mock_link_server};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server_pack};
		HardSyncMutex<int> mutex {&node, client, mutex_server};

		void SetUp() override {
			ON_CALL(node, state).WillByDefault(ReturnPointee(&state));
			ON_CALL(node, getLocation).WillByDefault(ReturnPointee(&location));
			mutex_server.manage(DistributedId(3, comm.getRank()), &mutex);

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
		TypedMpi<Color> color_mpi {comm};
		TerminationAlgorithm termination {comm, color_mpi};
		TypedMpi<DistributedId> id_mpi {comm};
		TypedMpi<int> data_mpi {comm};
		TypedMpi<DataUpdatePack<int>> data_update_mpi {comm};

		int data = 0;
		MockDistributedNode<int> node {DistributedId(3, 6), data};

		LocationState state = LocationState::DISTANT;
		int location = 0;

		MockLinkServer mock_link_server;
		MutexServer<int> mutex_server {comm, id_mpi, data_mpi, data_update_mpi, mock_link_server};
		fpmas::synchro::hard::ServerPack<int> server_pack {comm, termination, mutex_server, mock_link_server};
		MutexClient<int> client {comm, id_mpi, data_mpi, data_update_mpi, server_pack};
		HardSyncMutex<int> mutex {&node, client, mutex_server};

		void SetUp() override {
			EXPECT_CALL(mock_link_server, handleIncomingRequests).Times(AnyNumber());
			ON_CALL(node, state).WillByDefault(ReturnPointee(&state));
			ON_CALL(node, getLocation).WillByDefault(ReturnPointee(&location));

			mutex_server.manage(DistributedId(3, 6), &mutex);
			FPMAS_ON_PROC(comm, 0) {
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

	FPMAS_ON_PROC(comm, 0) {
		ASSERT_EQ(mutex.read(), comm.getSize() * NUM_ACQUIRE);
	}
}

class HardSyncModeIntegrationTest : public ::testing::Test {
	protected:
		MpiCommunicator comm;
		DistributedGraph<
			unsigned int, HardSyncMode,
			DistributedNode,
			DistributedEdge,
			TypedMpi,
			LocationManager
			> graph {comm};

		std::mt19937 engine;
		
		fpmas::api::load_balancing::PartitionMap partition;
};

class HardSyncModeMutexIntegrationTest : public HardSyncModeIntegrationTest {
	protected:
		void SetUp() override {
			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < comm.getSize(); i++) {
					auto* node = graph.buildNode(0ul);
					partition[node->getId()] = i;
				}
				// Builds a complete graph
				for(auto source : graph.getNodes()) {
					for(auto target : graph.getNodes()) {
						if(source.second != target.second) {
							graph.link(source.second, target.second, 0);
						}
					}
				}
			}
			graph.distribute(partition);
		}
};

TEST_F(HardSyncModeMutexIntegrationTest, acquire) {
	ASSERT_THAT(graph.getLocationManager().getLocalNodes(), SizeIs(1));

	FPMAS_MIN_PROCS("HardSyncModeIntegrationTest.acquire", comm, 2) {
		unsigned int num_writes = 10000;
		unsigned int local_writes_count = 0;
		for(unsigned int i = 0; i < num_writes; i++) {
			for(auto node : graph.getLocationManager().getLocalNodes()) {
				auto& out_nodes = node.second->outNeighbors();
				std::uniform_int_distribution<std::size_t> dist(0, out_nodes.size()-1);
				std::size_t random_index = dist(engine);

				auto* out_node = out_nodes.at(random_index);
				out_node->mutex()->acquire();
				out_node->data()++;
				out_node->mutex()->releaseAcquire();
				local_writes_count++;
			}
		}

		// Synchronization barrier
		graph.synchronize();

		unsigned int local_node_sum = 0;
		for(auto node : graph.getLocationManager().getLocalNodes()) {
			local_node_sum+=node.second->data();
		}

		fpmas::communication::TypedMpi<unsigned int> mpi {comm};
		std::vector<unsigned int> total_writes = mpi.gather(local_writes_count, 0);
		unsigned int write_sum = std::accumulate(total_writes.begin(), total_writes.end(), 0);

		std::vector<unsigned int> total_node_sum = mpi.gather(local_node_sum, 0);
		unsigned int node_sum = std::accumulate(total_node_sum.begin(), total_node_sum.end(), 0);

		FPMAS_ON_PROC(comm, 0) {
			ASSERT_EQ(write_sum, node_sum);
		}
	}
}

class HardSyncModeLinkerIntegrationTest : public HardSyncModeIntegrationTest {
	protected:
		void SetUp() override {
			FPMAS_ON_PROC(comm, 0) {
				std::vector<DistributedId> node_ids;
				for(int i = 0; i < comm.getSize(); i++) {
					auto* node = graph.buildNode(0ul);
					partition[node->getId()] = i;
					node_ids.push_back(node->getId());
				}
				for(std::size_t i = 0; i < node_ids.size() - 1; i ++) {
					graph.link(
						graph.getNode(node_ids[i]),
						graph.getNode(node_ids[i+1]),
						0);
				}
				graph.link(
						graph.getNode(node_ids[node_ids.size()-1]),
						graph.getNode(node_ids[0]),
						0);
			}
			graph.distribute(partition);
		}
};

TEST_F(HardSyncModeLinkerIntegrationTest, remove_node) {
	ASSERT_THAT(graph.getNodes(), SizeIs(Ge(1)));
	if(comm.getSize() > 1) {
		for(auto node : graph.getLocationManager().getLocalNodes()) {
			for(auto neighbor : node.second->outNeighbors())
				graph.removeNode(neighbor);
		}
	} else {
		decltype(graph)::NodeMap nodes = graph.getNodes();
		for(auto node : nodes)
			graph.removeNode(node.second);
	}

	graph.synchronize();

	ASSERT_THAT(graph.getNodes(), IsEmpty());
	ASSERT_THAT(graph.getEdges(), IsEmpty());
}
