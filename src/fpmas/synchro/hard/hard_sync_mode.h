#ifndef BASIC_HARD_SYNC_MODE_H
#define BASIC_HARD_SYNC_MODE_H

#include "fpmas/api/synchro/sync_mode.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/parallel/distributed_node.h"
#include "hard_data_sync.h"
#include "hard_sync_mutex.h"
#include "hard_sync_linker.h"
#include "mutex_server.h"
#include "mutex_client.h"
#include "termination.h"
#include "server_pack.h"

namespace fpmas::synchro {

	namespace hard {
		template<typename T>
			class HardSyncRuntime : public fpmas::api::synchro::SyncModeRuntime<T> {

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

				ServerPack<T> server_pack;
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
					server_pack(termination, mutex_server, link_server),
					sync_linker(comm, link_client, server_pack),
					data_sync(comm, server_pack) {}

				HardSyncMutex<T>* buildMutex(api::graph::parallel::DistributedNode<T>* node) override {
					HardSyncMutex<T>* mutex = new HardSyncMutex<T>(node, mutex_client, mutex_server);
					mutex_server.manage(node->getId(), mutex);
					return mutex;
				};

				HardDataSync<T>& getDataSync() override {return data_sync;};
				HardSyncLinker<T>& getSyncLinker() override {return sync_linker;};
			};

	}
	typedef fpmas::api::synchro::SyncMode<hard::HardSyncMutex, hard::HardSyncRuntime> HardSyncMode;
}
#endif
