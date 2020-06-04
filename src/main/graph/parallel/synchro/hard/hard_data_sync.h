#ifndef HARD_DATA_SYNC_H
#define HARD_DATA_SYNC_H

#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/sync_mode.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {

	template<typename NodeType, typename ArcType, typename TerminationAlgorithm>
		class HardDataSync : public api::graph::parallel::synchro::DataSync {
			typedef
			FPMAS::api::graph::parallel::synchro::hard::MutexServer<typename NodeType::DataType>
				MutexServer;

			TerminationAlgorithm termination;
			MutexServer& mutex_server;

			public:
				HardDataSync(FPMAS::api::communication::MpiCommunicator& comm, MutexServer& server)
					: termination(comm), mutex_server(server) {
				}

				const TerminationAlgorithm& getTerminationAlgorithm() const {return termination;}

				void synchronize() override {
					termination.terminate(mutex_server);
				};
		};

}

#endif
