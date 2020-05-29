#ifndef BASIC_HARD_SYNC_MODE_H
#define BASIC_HARD_SYNC_MODE_H

#include "api/graph/parallel/synchro/sync_mode.h"
#include "communication/communication.h"
#include "hard_data_sync.h"
#include "hard_sync_mutex.h"
#include "hard_sync_linker.h"
#include "mutex_server.h"
#include "mutex_client.h"
#include "termination.h"

namespace FPMAS::graph::parallel::synchro::hard {

	template<typename Node, typename Arc, typename Mutex>
		class HardSyncRuntime : public FPMAS::api::graph::parallel::synchro::SyncModeRuntime<Node, Arc, Mutex> {

			typedef FPMAS::graph::parallel::synchro::hard::TerminationAlgorithm<
				communication::TypedMpi>
				TerminationAlgorithm;

			MutexServer<typename Node::Data, communication::TypedMpi> mutexServer;
			MutexClient<typename Node::Data, communication::TypedMpi> mutexClient;

			LinkServer<Node, Arc, communication::TypedMpi> linkServer;
			LinkClient<Arc, communication::TypedMpi> linkClient;

			HardSyncLinker<Arc, TerminationAlgorithm> syncLinker;
			HardDataSync<Node, Arc, TerminationAlgorithm> dataSync;

			public:
				HardSyncRuntime(
						FPMAS::api::graph::parallel::DistributedGraph<Node, Arc>& graph,
						FPMAS::api::communication::MpiCommunicator& comm) :
					mutexServer(comm), mutexClient(comm, mutexServer),
					linkServer(comm, graph), linkClient(comm, linkServer),
					syncLinker(comm, linkClient, linkServer),
					dataSync(comm, mutexServer) {}

				void setUp(DistributedId id, Mutex& mutex) override {
					mutexServer.manage(id, &mutex);
				};

				HardDataSync<Node, Arc, TerminationAlgorithm>& getDataSync() override {return dataSync;};
				HardSyncLinker<Arc, TerminationAlgorithm>& getSyncLinker() override {return syncLinker;};
		};

	typedef FPMAS::api::graph::parallel::synchro::SyncMode<HardSyncMutex, HardSyncRuntime> HardSyncMode;
}

#endif
