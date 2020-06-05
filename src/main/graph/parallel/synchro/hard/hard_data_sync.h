#ifndef HARD_DATA_SYNC_H
#define HARD_DATA_SYNC_H

#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/sync_mode.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {

	template<typename T>
		class HardDataSync : public api::graph::parallel::synchro::DataSync {
			typedef api::graph::parallel::synchro::hard::MutexServer<T>
				MutexServer;
			typedef api::graph::parallel::synchro::hard::TerminationAlgorithm
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
