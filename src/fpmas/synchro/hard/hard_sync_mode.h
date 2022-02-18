#ifndef FPMAS_HARD_SYNC_MODE_H
#define FPMAS_HARD_SYNC_MODE_H

/** \file src/fpmas/synchro/hard/hard_sync_mode.h
 * HardSyncMode implementation.
 */

#include "fpmas/communication/communication.h"
#include "hard_data_sync.h"
#include "hard_sync_mutex.h"
#include "hard_sync_linker.h"

namespace fpmas { namespace synchro {
	namespace hard {
		/**
		 * Defines a generic HardSyncMode base, i.e. a mode where read() and
		 * acquire() operations are performed on the fly using point-to-point
		 * MPI calls, what is permitted by the HardDataSync usage.
		 *
		 * The SyncLinker is not yet defined in this class, as link operations
		 * management can be performed on the fly or using collective
		 * communications.
		 *
		 * @see fpmas::synchro::hard::hard_link::HardSyncLinker
		 * @see fpmas::synchro::hard::hard_link::HardSyncMode
		 * @see fpmas::synchro::hard::ghost_link::HardSyncLinker
		 * @see fpmas::synchro::hard::ghost_link::HardSyncMode
		 */
		template<typename T>
			class HardSyncModeBase : public fpmas::api::synchro::SyncMode<T> {
				private:
					communication::TypedMpi<Color> color_mpi;
					communication::TypedMpi<DistributedId> id_mpi;
					communication::TypedMpi<T> data_mpi;
					communication::TypedMpi<DataUpdatePack<T>> data_update_mpi;

				protected:
					/**
					 * A TerminationAlgorithm instance that can be used to
					 * terminate ServerPacks.
					 */
					TerminationAlgorithm termination;

					/**
					 * The MutexServer instance used by the internal
					 * HardDataSync component.
					 */
					MutexServer<T> mutex_server;

				private:
					MutexClient<T> mutex_client;
					HardDataSync<T> data_sync;

				public:

					/**
					 * HardSyncModeBase constructor.
					 *
					 * @param graph reference to managed graph
					 * @param comm MPI communicator
					 * @param link_server HardSyncLinker server that must be
					 * associated to the mutex_server. Might be a LinkServer or
					 * a VoidServer, depending on the current link operations
					 * management implementation.
					 * @param server_pack the ServerPack instance used for
					 * HardDataSync synchronization.
					 */
					HardSyncModeBase(
							fpmas::api::graph::DistributedGraph<T>& graph,
							fpmas::api::communication::MpiCommunicator& comm,
							api::Server& link_server,
							ServerPackBase& server_pack
							) :
						color_mpi(comm), id_mpi(comm),
						data_mpi(comm), data_update_mpi(comm),
						termination(comm, color_mpi),
						mutex_server(comm, id_mpi, data_mpi, data_update_mpi, link_server),
						mutex_client(comm, id_mpi, data_mpi, data_update_mpi, server_pack),
						data_sync(comm, server_pack, graph) {
						}

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

			};

		namespace hard_link {
			/**
			 * Hard synchronization mode implementation.
			 *
			 * hard_link::HardSyncMode defines the strongest level of graph
			 * synchronization.
			 *
			 * At each \DISTANT node data access, the HardSyncMutex instance performs
			 * distant communication with host processes to ensure :
			 * 1. accessed data is **always** up to date
			 * 2. a strict concurrent access management at the global execution
			 * level
			 *
			 * Moreover, link, unlink and node removal operations are committed on
			 * the fly by the hard_link::HardSyncLinker instance.
			 *
			 * A TerminationAlgorithm is used to finalize synchronization
			 * operations.
			 */
			template<typename T>
				class HardSyncMode : public HardSyncModeBase<T> {
					private:
						communication::TypedMpi<DistributedId> id_mpi;
						communication::TypedMpi<graph::EdgePtrWrapper<T>> edge_mpi;

						LinkServer<T> link_server;
						LinkClient<T> link_client;

						ServerPack<api::MutexServer<T>, api::LinkServer> server_pack;
						HardSyncLinker<T> sync_linker;

					public:
						/**
						 * HardSyncMode contructor.
						 *
						 * @param graph reference to managed graph
						 * @param comm MPI communicator
						 */
						HardSyncMode(
								fpmas::api::graph::DistributedGraph<T>& graph,
								fpmas::api::communication::MpiCommunicator& comm) :
							HardSyncModeBase<T>(graph, comm, link_server, server_pack),
							id_mpi(comm), edge_mpi(comm),
							link_server(comm, graph, id_mpi, edge_mpi),
							link_client(comm, id_mpi, edge_mpi, server_pack),
							server_pack(
									comm, this->termination,
									this->mutex_server, link_server),
							sync_linker(graph, link_client, server_pack) {
							}
						/**
						 * Returns a reference to the internal HardSyncLinker instance.
						 *
						 * @return reference to the SyncLinker instance
						 */
						HardSyncLinker<T>& getSyncLinker() override {
							return sync_linker;
						};
				};
		}

		namespace ghost_link {
			/**
			 * Hard synchronization mode implementation.
			 *
			 * ghost_link::HardSyncMode defines the strongest level of graph
			 * data synchronization, but relaxes the link operations
			 * synchronization.
			 *
			 * At each \DISTANT node data access, the HardSyncMutex instance performs
			 * distant communication with host processes to ensure :
			 * 1. accessed data is **always** up to date
			 * 2. a strict concurrent access management at the global execution
			 * level
			 *
			 * Contrary to hard_link::HardSyncMode, \DISTANT link() and
			 * unlink() operations are performed using collective
			 * communications upon synchronization, using the
			 * ghost_link::HardSyncLinker component, what is likely to be more
			 * efficient.
			 *
			 * A TerminationAlgorithm is still used to finalize data
			 * synchronization operations.
			 */
			template<typename T>
				class HardSyncMode : public HardSyncModeBase<T> {
					private:
						communication::TypedMpi<DistributedId> id_mpi;
						communication::TypedMpi<graph::EdgePtrWrapper<T>> edge_mpi;

						// Unused link_server
						VoidServer link_server;
						// This server is terminated by HardGhostSyncLinker and
						// HardDataSync
						ServerPack<api::MutexServer<T>, VoidServer> server_pack;

						HardSyncLinker<T> sync_linker;


					public:
						/**
						 * HardSyncMode constructor.
						 *
						 * @param graph reference to managed graph
						 * @param comm MPI communicator
						 */
						HardSyncMode(
								fpmas::api::graph::DistributedGraph<T>& graph,
								fpmas::api::communication::MpiCommunicator& comm) :
							HardSyncModeBase<T>(graph, comm, link_server, server_pack),
							id_mpi(comm), edge_mpi(comm),
							server_pack(comm, this->termination, this->mutex_server, link_server),
							sync_linker(edge_mpi, id_mpi, graph, server_pack) {
							}

						/**
						 * Returns a reference to the internal GhostSyncLinker instance.
						 *
						 * @return reference to the SyncLinker instance
						 */
						HardSyncLinker<T>& getSyncLinker() override {return sync_linker;};
				};
		}

	}
	/**
	 * Default HardSyncMode.
	 */
	template<typename T>
		using HardSyncMode = hard::hard_link::HardSyncMode<T>;

	/**
	 * HardSyncMode but link operations are managed as in GhostMode.
	 */
	template<typename T>
		using HardSyncModeWithGhostLink = hard::ghost_link::HardSyncMode<T>;
}}
#endif
