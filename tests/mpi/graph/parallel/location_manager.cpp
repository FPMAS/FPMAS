#include "graph/parallel/distributed_graph.h"

#include <random>
#include "../mocks/load_balancing/mock_load_balancing.h"
#include "../mocks/synchro/mock_sync_mode.h"

using ::testing::ReturnRef;
using ::testing::SizeIs;
using ::testing::Ge;

using fpmas::graph::parallel::DistributedGraph;
using fpmas::graph::parallel::DistributedNode;
using fpmas::graph::parallel::DistributedArc;
using fpmas::graph::parallel::LocationManager;
using fpmas::graph::parallel::DefaultMpiSetUp;

template<typename T>
class FakeMutex : public fpmas::api::synchro::Mutex<T> {
	private:
		std::reference_wrapper<T> _data;
		void _lock() override {}
		void _unlock() override {}
		void _lockShared() override {}
		void _unlockShared() override {}

	public:
		FakeMutex(T& data) : _data(data) {}
		T& data() override {return _data;}
		const T& data() const override {return _data;}

		const T& read() override {return _data;}
		void releaseRead() override {}

		T& acquire() override {return _data;}
		void releaseAcquire() override {}

		void lock() override {}
		void unlock() override {}
		bool locked() const override {return false;}

		void lockShared() override {}
		void unlockShared() override {}
		int lockedShared() const override {return 0;}

};

class LocationManagerIntegrationTest : public ::testing::Test {
	protected:
		inline static const int SEQUENCE_COUNT = 5;
		inline static const int NODES_COUNT = 100;

		DistributedGraph<
			int, MockSyncMode<FakeMutex>,
			DistributedNode,
			DistributedArc,
			DefaultMpiSetUp,
			LocationManager
			> graph;

		MockDataSync data_sync;
		MockSyncLinker<int> sync_linker;

		std::mt19937 engine;
		std::uniform_int_distribution<int> dist {0, graph.getMpiCommunicator().getSize()-1};
		
		typename decltype(graph)::PartitionMap partition;

		std::array<std::array<int, SEQUENCE_COUNT>, NODES_COUNT> location_sequences;

		void SetUp() override {
			EXPECT_CALL(sync_linker, link).Times(AnyNumber());
			ON_CALL(graph.getSyncModeRuntime(), getSyncLinker).WillByDefault(ReturnRef(sync_linker));
			EXPECT_CALL(graph.getSyncModeRuntime(), getSyncLinker).Times(AnyNumber());
			ON_CALL(graph.getSyncModeRuntime(), getDataSync).WillByDefault(ReturnRef(data_sync));
			EXPECT_CALL(graph.getSyncModeRuntime(), getDataSync).Times(AnyNumber());

			for(int i = 0; i < SEQUENCE_COUNT; i++) {
				for(int j = 0; j < NODES_COUNT ; j++) {
					location_sequences[j][i] = dist(engine);
				}
			}

			EXPECT_CALL(graph.getSyncModeRuntime(), buildMutex).Times(AnyNumber());
			if(graph.getMpiCommunicator().getRank() == 0) {
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
				if(node.second->state() == fpmas::graph::parallel::LocationState::LOCAL) {
					localNodeCount++;
					ASSERT_THAT(node.second->getIncomingArcs(), SizeIs(NODES_COUNT-1));
					ASSERT_THAT(node.second->getOutgoingArcs(), SizeIs(NODES_COUNT-1));
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
