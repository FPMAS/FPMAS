#ifndef FPMAS_HARD_DATA_SYNC_H
#define FPMAS_HARD_DATA_SYNC_H

/** \file src/fpmas/synchro/hard/hard_data_sync.h
 * HardSyncMode DataSync implementation.
 */

#include "fpmas/api/graph/distributed_graph.h"
#include "server_pack.h"

namespace fpmas { namespace synchro { namespace hard {

	/**
	 * HardSyncMode fpmas::api::synchro::DataSync implementation.
	 *
	 * Since data is always fetched directly from distance processes by the
	 * HardSyncMutex, no real data synchronization is required in this
	 * implementation. However, a termination algorithm must be applied in the
	 * synchronization process to handle incoming requests from other processes
	 * until global termination (i.e. global synchronization) is determined.
	 */
	template<typename T>
		class HardDataSync : public fpmas::api::synchro::DataSync<T> {
			typedef api::MutexServer<T> MutexServer;
			typedef api::TerminationAlgorithm TerminationAlgorithm;

			fpmas::api::communication::MpiCommunicator& comm;
			ServerPackBase& server_pack;
			fpmas::api::graph::DistributedGraph<T>& graph;

			public:
			/**
			 * HardDataSync constructor.
			 *
			 * @param comm MPI communicator
			 * @param server_pack associated server pack
			 * @param graph associated graph
			 */
			HardDataSync(
					fpmas::api::communication::MpiCommunicator& comm,
					ServerPackBase& server_pack,
					fpmas::api::graph::DistributedGraph<T>& graph)
				: comm(comm), server_pack(server_pack), graph(graph) {
				}

			/**
			 * Synchronizes all the processes using
			 * ServerPack::terminate().
			 *
			 * Notice that, since ServerPack is used, not only HardSyncMutex
			 * requests (i.e. data related requests) but also SyncLinker
			 * requests will be handled until termination.
			 *
			 * This is required to avoid the following deadlock situation :
			 * | Process 0                      | Process 1
			 * |--------------------------------|---------------------------
			 * | ...                            | ...
			 * | init HardDataSync::terminate() | send link request to P0...
			 * | handles data requests...       | waits for link response...
			 * | handles data requests...       | waits for link response...
			 * | DEADLOCK                       ||
			 */
			void synchronize() override {
				FPMAS_LOGI(comm.getRank(), "HARD_DATA_SYNC", "Synchronizing data sync...", "");
				server_pack.terminate();
				FPMAS_LOGI(comm.getRank(), "HARD_DATA_SYNC", "Synchronized.", "");
			};

			/**
			 * Same as synchronize().
			 *
			 * Indeed, partial synchronization is not really relevant in the
			 * case of HardSyncMode, since data exchanges in this mode occur
			 * only when required, when read() or acquire() method are called,
			 * but not directly when the termination algorithm is performed.
			 *
			 * So we can consider that all nodes are synchronized in any case
			 * when this method is called.
			 */
			void synchronize(
					std::unordered_set<fpmas::api::graph::DistributedNode<T>*>
					) override {
				synchronize();
			}
		};

}}}
#endif
