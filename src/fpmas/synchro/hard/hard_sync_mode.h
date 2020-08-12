#ifndef BASIC_HARD_SYNC_MODE_H
#define BASIC_HARD_SYNC_MODE_H

#include "fpmas/api/synchro/sync_mode.h"
#include "fpmas/communication/communication.h"
#include "fpmas/graph/distributed_node.h"
#include "hard_data_sync.h"
#include "hard_sync_mutex.h"
#include "hard_sync_linker.h"
#include "mutex_server.h"
#include "mutex_client.h"
#include "termination.h"
#include "server_pack.h"

namespace fpmas { namespace synchro {
	namespace hard {
		template<typename T>
			class HardSyncMode : public fpmas::api::synchro::SyncMode<T> {

				typedef synchro::hard::TerminationAlgorithm<
					communication::TypedMpi>
					TerminationAlgorithm;

				communication::TypedMpi<DistributedId> id_mpi;
				communication::TypedMpi<T> data_mpi;
				communication::TypedMpi<DataUpdatePack<T>> data_update_mpi;
				communication::TypedMpi<graph::NodePtrWrapper<T>> node_mpi;
				communication::TypedMpi<graph::EdgePtrWrapper<T>> edge_mpi;

				MutexServer<T> mutex_server;
				MutexClient<T> mutex_client;

				LinkServer<T> link_server;
				LinkClient<T> link_client;
				TerminationAlgorithm termination;

				ServerPack<T> server_pack;
				HardSyncLinker<T> sync_linker;
				HardDataSync<T> data_sync;

				public:
				HardSyncMode(
						fpmas::api::graph::DistributedGraph<T>& graph,
						fpmas::api::communication::MpiCommunicator& comm) :
					id_mpi(comm), data_mpi(comm), data_update_mpi(comm), node_mpi(comm), edge_mpi(comm),
					mutex_server(comm, id_mpi, data_mpi, data_update_mpi),
					mutex_client(comm, id_mpi, data_mpi, data_update_mpi, server_pack),
					link_server(comm, graph, id_mpi, edge_mpi),
					link_client(comm, id_mpi, edge_mpi, server_pack),
					termination(comm),
					server_pack(comm, termination, mutex_server, link_server),
					sync_linker(graph, link_client, server_pack),
					data_sync(comm, server_pack) {}

				HardSyncMutex<T>* buildMutex(fpmas::api::graph::DistributedNode<T>* node) override {
					HardSyncMutex<T>* mutex = new HardSyncMutex<T>(node, mutex_client, mutex_server);
					mutex_server.manage(node->getId(), mutex);
					return mutex;
				};

				HardDataSync<T>& getDataSync() override {return data_sync;};
				HardSyncLinker<T>& getSyncLinker() override {return sync_linker;};
			};

	}
	using hard::HardSyncMode;
}}
#endif
