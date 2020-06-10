#ifndef HARD_DATA_SYNC_H
#define HARD_DATA_SYNC_H

#include "api/communication/communication.h"
#include "api/synchro/sync_mode.h"
#include "api/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::synchro::hard {

	template<typename T>
		class HardDataSync : public api::synchro::DataSync {
			typedef api::synchro::hard::MutexServer<T>
				MutexServer;
			typedef api::synchro::hard::TerminationAlgorithm
				TerminationAlgorithm;

			MutexServer& mutex_server;
			TerminationAlgorithm& termination;

			public:
				HardDataSync(MutexServer& server, TerminationAlgorithm& termination)
					: mutex_server(server), termination(termination) {
				}

				void synchronize() override {
					termination.terminate(mutex_server);
				};
		};

}

#endif
