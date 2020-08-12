#ifndef HARD_DATA_SYNC_H
#define HARD_DATA_SYNC_H

#include "fpmas/api/communication/communication.h"
#include "fpmas/api/synchro/sync_mode.h"
#include "fpmas/synchro/hard/api/hard_sync_mutex.h"
#include "server_pack.h"

namespace fpmas { namespace synchro { namespace hard {

	template<typename T>
		class HardDataSync : public fpmas::api::synchro::DataSync {
			typedef api::MutexServer<T> MutexServer;
			typedef api::TerminationAlgorithm TerminationAlgorithm;

			fpmas::api::communication::MpiCommunicator& comm;
			ServerPack<T>& server_pack;

			public:
				HardDataSync(fpmas::api::communication::MpiCommunicator& comm, ServerPack<T>& server_pack)
					: comm(comm), server_pack(server_pack) {
				}

				void synchronize() override {
					FPMAS_LOGI(comm.getRank(), "HARD_DATA_SYNC", "Synchronizing data sync...");
					server_pack.terminate();
					//termination.terminate(mutex_server);
					FPMAS_LOGI(comm.getRank(), "HARD_DATA_SYNC", "Synchronized.");
				};
		};

}}}
#endif
