#include "fpmas/graph/distributed_graph.h"

#include <random>
#include "../mocks/graph/mock_load_balancing.h"
#include "../mocks/synchro/mock_sync_mode.h"

using ::testing::ReturnRef;
using ::testing::SizeIs;
using ::testing::Ge;

using fpmas::graph::DistributedGraph;
using fpmas::communication::MpiCommunicator;
using fpmas::communication::TypedMpi;

class LocationManagerIntegrationTest : public ::testing::Test {
	protected:
		static const int SEQUENCE_COUNT = 5;
		static const int NODES_COUNT = 100;

		MpiCommunicator comm;
		DistributedGraph<int, MockSyncMode> graph {comm};

		MockDataSync data_sync;
		MockSyncLinker<int> sync_linker;

		std::mt19937 engine;
		std::uniform_int_distribution<int> dist {0, graph.getMpiCommunicator().getSize()-1};
		
		typename fpmas::api::graph::PartitionMap partition;

		std::array<std::array<int, SEQUENCE_COUNT>, NODES_COUNT> location_sequences;

		void SetUp() override {
			EXPECT_CALL(sync_linker, link).Times(AnyNumber());
			ON_CALL(graph.getSyncMode(), getSyncLinker).WillByDefault(ReturnRef(sync_linker));
			EXPECT_CALL(graph.getSyncMode(), getSyncLinker).Times(AnyNumber());
			ON_CALL(graph.getSyncMode(), getDataSync).WillByDefault(ReturnRef(data_sync));
			EXPECT_CALL(graph.getSyncMode(), getDataSync).Times(AnyNumber());

			for(int i = 0; i < SEQUENCE_COUNT; i++) {
				for(int j = 0; j < NODES_COUNT ; j++) {
					location_sequences[j][i] = dist(engine);
				}
			}

			EXPECT_CALL(graph.getSyncMode(), buildMutex).Times(AnyNumber());
			FPMAS_ON_PROC(comm, 0) {
				for(int i = 0; i < NODES_COUNT; i++) {
					graph.buildNode();
				}
				for(auto src : graph.getNodes()) {
					for(auto tgt : graph.getNodes()) {
						if(src.first != tgt.first) {
							graph.link(src.second, tgt.second, 0);
						}
					}
				}
			}
		}
		void generatePartition(int i) {
			partition.clear();
			for(int j = 0; j < NODES_COUNT; j++) {
				partition[DistributedId(0, j)] = location_sequences[j][i];
			}
		}
		void checkPartition(int i) {
			FPMAS_LOGD(graph.getMpiCommunicator().getRank(), "TEST", "check %i", i);
			int nodeCount = 0; // Number of nodes that will be contained in the graph after the next distribute call 
			for(int j = 0; j < NODES_COUNT; j++) {
				if(location_sequences[j][i] == graph.getMpiCommunicator().getRank())
					nodeCount++;
			}
			ASSERT_THAT(graph.getNodes(), SizeIs(nodeCount > 0 ? NODES_COUNT : 0));
			int localNodeCount = 0;
			for(auto node : graph.getNodes()) {
				if(node.second->state() == fpmas::graph::LocationState::LOCAL) {
					localNodeCount++;
					ASSERT_THAT(node.second->getIncomingEdges(), SizeIs(NODES_COUNT-1));
					ASSERT_THAT(node.second->getOutgoingEdges(), SizeIs(NODES_COUNT-1));
				}
			}
			ASSERT_EQ(nodeCount, localNodeCount);

			for(auto node : graph.getNodes()) {
				ASSERT_EQ(node.second->getLocation(), location_sequences[node.first.id()][i]);
			}
			FPMAS_LOGD(graph.getMpiCommunicator().getRank(), "TEST", "done %i", i);
		}

};

TEST_F(LocationManagerIntegrationTest, location_updates) {
	for(int i = 0; i < SEQUENCE_COUNT; i++) {
		{ ::testing::InSequence s;
		EXPECT_CALL(sync_linker, synchronize);
		EXPECT_CALL(data_sync, synchronize);
		}
		generatePartition(i);

		FPMAS_LOGD(graph.getMpiCommunicator().getRank(), "TEST", "Dist %i", i);
		graph.distribute(partition);
		FPMAS_LOGD(graph.getMpiCommunicator().getRank(), "TEST", "Dist %i done", i);
		checkPartition(i);
	}
}
