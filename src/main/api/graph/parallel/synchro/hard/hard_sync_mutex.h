#ifndef HARD_REQUEST_HANDLER_H
#define HARD_REQUEST_HANDLER_H

#include <queue>
#include "../mutex.h"
#include "client_server.h"

namespace FPMAS::api::graph::parallel::synchro::hard {

	template<typename T>
		class HardSyncMutex : public virtual synchro::Mutex<T> {
			friend MutexServer<T>;
			public:
				virtual void pushRequest(Request) = 0;
				virtual std::queue<Request> requestsToProcess() = 0;

				virtual ~HardSyncMutex() {}
		};

}
#endif
