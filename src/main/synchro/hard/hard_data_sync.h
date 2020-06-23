#ifndef HARD_DATA_SYNC_H
#define HARD_DATA_SYNC_H

#include "api/communication/communication.h"
#include "api/synchro/sync_mode.h"
#include "api/synchro/hard/hard_sync_mutex.h"
#include "server_pack.h"

namespace FPMAS::synchro::hard {

	template<typename T>
		class HardDataSync : public api::synchro::DataSync {
			typedef api::synchro::hard::MutexServer<T>
				MutexServer;
			typedef api::synchro::hard::TerminationAlgorithm
				TerminationAlgorithm;

			api::communication::MpiCommunicator& comm;
			ServerPack<T>& server_pack;

			public:
				HardDataSync(api::communication::MpiCommunicator& comm, ServerPack<T>& server_pack)
					: comm(comm), server_pack(server_pack) {
				}

				void synchronize() override {
					FPMAS_LOGI(comm.getRank(), "HARD_DATA_SYNC", "Synchronizing data sync...");
					server_pack.terminate();
					//termination.terminate(mutex_server);
					FPMAS_LOGI(comm.getRank(), "HARD_DATA_SYNC", "Synchronized.");
				};
		};

}

#endif
