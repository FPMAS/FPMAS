#ifndef HARD_REQUEST_HANDLER_H
#define HARD_REQUEST_HANDLER_H

#include <queue>
#include "../mutex.h"
#include "client_server.h"

namespace FPMAS::api::synchro::hard {

	template<typename T>
		class HardSyncMutex : public virtual synchro::Mutex<T> {
			friend MutexServer<T>;
			public:
				virtual void pushRequest(MutexRequest) = 0;
				virtual std::queue<MutexRequest> requestsToProcess() = 0;

				virtual ~HardSyncMutex() {}
		};

}
#endif
