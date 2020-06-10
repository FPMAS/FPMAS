#ifndef BASIC_HARD_SYNC_MODE_H
#define BASIC_HARD_SYNC_MODE_H

#include "api/synchro/sync_mode.h"
#include "communication/communication.h"
#include "hard_data_sync.h"
#include "hard_sync_mutex.h"
#include "hard_sync_linker.h"
#include "mutex_server.h"
#include "mutex_client.h"
#include "termination.h"
#include "graph/parallel/distributed_node.h"

namespace FPMAS::synchro::hard {

	template<typename T>
		class HardSyncRuntime : public FPMAS::api::synchro::SyncModeRuntime<T> {

			typedef synchro::hard::TerminationAlgorithm<
				communication::TypedMpi>
				TerminationAlgorithm;

			communication::TypedMpi<DistributedId> id_mpi;
			communication::TypedMpi<T> data_mpi;
			communication::TypedMpi<DataUpdatePack<T>> data_update_mpi;
			communication::TypedMpi<graph::parallel::NodePtrWrapper<T>> node_mpi;
			communication::TypedMpi<graph::parallel::ArcPtrWrapper<T>> arc_mpi;

			MutexServer<T> mutex_server;
			MutexClient<T> mutex_client;

			LinkServer<T> link_server;
			LinkClient<T> link_client;

			TerminationAlgorithm termination;
			HardSyncLinker<T> sync_linker;
			HardDataSync<T> data_sync;

			public:
				HardSyncRuntime(
						api::graph::parallel::DistributedGraph<T>& graph,
						api::communication::MpiCommunicator& comm) :
					id_mpi(comm), data_mpi(comm), data_update_mpi(comm), node_mpi(comm), arc_mpi(comm),
					mutex_server(comm, id_mpi, data_mpi, data_update_mpi),
					mutex_client(comm, id_mpi, data_mpi, data_update_mpi, mutex_server),
					link_server(comm, id_mpi, arc_mpi, graph),
					link_client(comm, id_mpi, arc_mpi, link_server),
					termination(comm),
					sync_linker(link_client, link_server, termination),
					data_sync(mutex_server, termination) {}

				HardSyncMutex<T>* buildMutex(DistributedId id, T& data) override {
					HardSyncMutex<T>* mutex = new HardSyncMutex<T>(data);
					mutex_server.manage(id, mutex);
					return mutex;
				};

				HardDataSync<T>& getDataSync() override {return data_sync;};
				HardSyncLinker<T>& getSyncLinker() override {return sync_linker;};
		};

	typedef FPMAS::api::synchro::SyncMode<HardSyncMutex, HardSyncRuntime> HardSyncMode;
}

#endif
