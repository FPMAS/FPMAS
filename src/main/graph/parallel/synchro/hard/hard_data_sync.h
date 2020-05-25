#ifndef HARD_DATA_SYNC_H
#define HARD_DATA_SYNC_H

#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/sync_mode.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {

	template<typename NodeType, typename ArcType, typename TerminationAlgorithm>
		class HardDataSync : public api::graph::parallel::synchro::DataSync {
			typedef
			FPMAS::api::graph::parallel::synchro::hard::MutexServer<typename NodeType::data_type>
				mutex_server;

			TerminationAlgorithm termination;
			mutex_server& mutexServer;

			public:
				HardDataSync(FPMAS::api::communication::MpiCommunicator& comm, mutex_server& server)
					: termination(comm), mutexServer(server) {
				}

				const TerminationAlgorithm& getTerminationAlgorithm() const {return termination;}

				void synchronize() override {
					termination.terminate(mutexServer);
				};
		};

}

#endif
