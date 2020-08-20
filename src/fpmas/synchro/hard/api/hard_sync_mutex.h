#ifndef FPMAS_HARD_SYNC_MUTEX_API_H
#define FPMAS_HARD_SYNC_MUTEX_API_H

/** \file src/fpmas/synchro/hard/api/hard_sync_mutex.h
 * HardSyncMutex API
 */

#include <queue>
#include "fpmas/api/synchro/mutex.h"
#include "client_server.h"

namespace fpmas { namespace synchro { namespace hard { namespace api {
	/**
	 * Mutex API extension to handle HardSyncMode.
	 *
	 * This API adds a request queue concept to the regular Mutex.  Each
	 * HardSyncMutex instance (and so each DistributedNode) is associated to a
	 * queue of lock, shared lock, read and acquire requests incoming from
	 * available processes, including the local process. It is the job of the
	 * MutexServer to enqueue and handle those requests.
	 */
	template<typename T>
		class HardSyncMutex : public virtual fpmas::api::synchro::Mutex<T> {
			friend MutexServer<T>;
			public:
			/**
			 * Pushes a pending request into the waiting queue.
			 *
			 * @param request mutex request to push
			 */
			virtual void pushRequest(MutexRequest request) = 0;
			/**
			 * Returns an ordered list of requests that can be handled,
			 * depending on the current mutex status.
			 *
			 * For example, if the mutex is locked, an empty queue might be
			 * returned. Else, read or lock shared requests can be
			 * returned, until the next pending acquire or lock request.
			 *
			 * In any case, the exact behavior is implementation defined
			 * and might depend about what priority is given to each
			 * request. (e.g. :
			 * https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem)
			 *
			 * @return an ordered queue of request that can safely be
			 * handled at the time of the call, depending on the current
			 * mutex status
			 */
			virtual std::queue<MutexRequest> requestsToProcess() = 0;

			virtual ~HardSyncMutex() {}
		};
}}}}
#endif
