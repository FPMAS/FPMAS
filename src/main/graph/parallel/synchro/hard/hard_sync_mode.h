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
				typename Node::data_type, communication::TypedMpi>
				termination_algorithm;
			MutexServer<typename Node::data_type, communication::TypedMpi> mutexServer;
			MutexClient<typename Node::data_type, communication::TypedMpi> mutexClient;

			HardDataSync<Node, Arc, termination_algorithm> dataSync;
			HardSyncLinker<Arc> syncLinker;

			public:
				HardSyncRuntime(
						FPMAS::api::graph::parallel::DistributedGraph<Node, Arc>& graph,
						FPMAS::api::communication::MpiCommunicator& comm)
					: mutexServer(comm), mutexClient(comm, mutexServer), dataSync(comm, mutexServer) {}

				void setUp(DistributedId id, Mutex& mutex) override {
					mutexServer.manage(id, &mutex);
				};

				HardDataSync<Node, Arc, termination_algorithm>& getDataSync() override {return dataSync;};
				HardSyncLinker<Arc>& getSyncLinker() override {return syncLinker;};
		};

	class HardSyncMode : public FPMAS::api::graph::parallel::synchro::SyncMode<HardSyncMutex, HardSyncRuntime> {

	};
}

#endif
