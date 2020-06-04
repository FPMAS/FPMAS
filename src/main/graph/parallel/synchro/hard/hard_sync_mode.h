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

			MutexServer<typename Node::DataType, communication::TypedMpi> mutex_server;
			MutexClient<typename Node::DataType, communication::TypedMpi> mutex_client;

			LinkServer<Node, Arc, communication::TypedMpi> link_server;
			LinkClient<Arc, communication::TypedMpi> link_client;

			HardSyncLinker<Arc, TerminationAlgorithm> sync_linker;
			HardDataSync<Node, Arc, TerminationAlgorithm> data_sync;

			public:
				HardSyncRuntime(
						FPMAS::api::graph::parallel::DistributedGraph<typename Node::DataType>& graph,
						FPMAS::api::communication::MpiCommunicator& comm) :
					mutex_server(comm), mutex_client(comm, mutex_server),
					link_server(comm, graph), link_client(comm, link_server),
					sync_linker(comm, link_client, link_server),
					data_sync(comm, mutex_server) {}

				void setUp(DistributedId id, Mutex& mutex) override {
					mutex_server.manage(id, &mutex);
				};

				HardDataSync<Node, Arc, TerminationAlgorithm>& getDataSync() override {return data_sync;};
				HardSyncLinker<Arc, TerminationAlgorithm>& getSyncLinker() override {return sync_linker;};
		};

	typedef FPMAS::api::graph::parallel::synchro::SyncMode<HardSyncMutex, HardSyncRuntime> HardSyncMode;
}

#endif
