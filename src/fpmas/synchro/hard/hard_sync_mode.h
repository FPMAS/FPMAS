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
		/**
		 * Hard synchronization SyncMode implementation.
		 *
		 * HardSyncMode defines the strongest level of graph synchronization.
		 *
		 * At each \DISTANT node data access, the HardSyncMutex instance performs
		 * distant communication with host processes to ensure :
		 * 1. accessed data is **always** up to date
		 * 2. a strict concurrent access management at the global execution
		 * level
		 *
		 * Moreover, link, unlink and node removal operations are committed on
		 * the fly by the HardSyncLinker instance.
		 *
		 * A TerminationAlgorithm is used to finalize synchronization
		 * operations.
		 */
		template<typename T>
			class HardSyncMode : public fpmas::api::synchro::SyncMode<T> {

				communication::TypedMpi<DistributedId> id_mpi;
				communication::TypedMpi<T> data_mpi;
				communication::TypedMpi<DataUpdatePack<T>> data_update_mpi;
				communication::TypedMpi<graph::NodePtrWrapper<T>> node_mpi;
				communication::TypedMpi<graph::EdgePtrWrapper<T>> edge_mpi;
				communication::TypedMpi<Color> color_mpi;

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
					id_mpi(comm), data_mpi(comm), data_update_mpi(comm),
					node_mpi(comm), edge_mpi(comm), color_mpi(comm),
					mutex_server(comm, id_mpi, data_mpi, data_update_mpi),
					mutex_client(comm, id_mpi, data_mpi, data_update_mpi, server_pack),
					link_server(comm, graph, id_mpi, edge_mpi),
					link_client(comm, id_mpi, edge_mpi, server_pack),
					termination(comm, color_mpi),
					server_pack(comm, termination, mutex_server, link_server),
					sync_linker(graph, link_client, server_pack),
					data_sync(comm, server_pack) {}

				/**
				 * Builds a new HardSyncMutex from the specified node data.
				 *
				 * @param node node to which the built mutex will be associated
				 */
				HardSyncMutex<T>* buildMutex(fpmas::api::graph::DistributedNode<T>* node) override {
					HardSyncMutex<T>* mutex = new HardSyncMutex<T>(node, mutex_client, mutex_server);
					mutex_server.manage(node->getId(), mutex);
					return mutex;
				};

				/**
				 * Returns a reference to the internal HardDataSync instance.
				 *
				 * @return reference to the DataSync instance
				 */
				HardDataSync<T>& getDataSync() override {return data_sync;};

				/**
				 * Returns a reference to the internal HardSyncLinker instance.
				 *
				 * @return reference to the SyncLinker instance
				 */
				HardSyncLinker<T>& getSyncLinker() override {return sync_linker;};
			};

	}
	using hard::HardSyncMode;
}}
#endif
