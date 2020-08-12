#ifndef HARD_REQUEST_HANDLER_H
#define HARD_REQUEST_HANDLER_H

#include <queue>
#include "fpmas/api/synchro/mutex.h"
#include "client_server.h"

namespace fpmas { namespace synchro {
	namespace hard { namespace api {
		/**
		 * Mutex API extension to handle HardSyncMode.
		 *
		 * This API adds a request queue concept to the regular Mutex.
		 */
		template<typename T>
			class HardSyncMutex : public virtual fpmas::api::synchro::Mutex<T> {
				friend MutexServer<T>;
				public:
				virtual void pushRequest(MutexRequest) = 0;
				virtual std::queue<MutexRequest> requestsToProcess() = 0;

				virtual ~HardSyncMutex() {}
			};
	}
}}}
#endif
