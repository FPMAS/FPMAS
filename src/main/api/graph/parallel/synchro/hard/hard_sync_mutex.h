#ifndef HARD_REQUEST_HANDLER_H
#define HARD_REQUEST_HANDLER_H

#include <queue>
#include "../mutex.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel::synchro::hard {
	struct Request {
		DistributedId id;
		int source;

		Request(DistributedId id, int source)
			: id(id), source(source) {}
	};

	template<typename T> class MutexClient;

	template<typename T>
		class HardSyncMutex : public virtual synchro::Mutex<T> {
			friend MutexClient<T>;
			public:
				virtual std::queue<Request>& readRequests() = 0;
				virtual std::queue<Request>& acquireRequests() = 0;

				virtual ~HardSyncMutex() {}
		};

	template<typename T>
	class MutexClient {
		friend HardSyncMutex<T>;
		protected:
			void lock(HardSyncMutex<T>* mutex) {mutex->_lock();}
			void unlock(HardSyncMutex<T>* mutex) {mutex->_lock();}
		public:
			virtual void manage(DistributedId id, HardSyncMutex<T>*) = 0;
			virtual void remove(DistributedId id) = 0;

			virtual T read(DistributedId, int location) = 0;
			virtual T acquire(DistributedId, int location) = 0;
			virtual void release(DistributedId, int location) = 0;

			virtual void lock(DistributedId, int location) = 0;
			virtual void unlock(DistributedId, int location) = 0;

			virtual ~MutexClient() {}
	};

	template<typename T>
	class MutexServer {
		public:
			virtual void manage(DistributedId id, HardSyncMutex<T>*) = 0;
			virtual void remove(DistributedId id) = 0;

			virtual void wait(const Request& req) = 0;
			virtual void notify(DistributedId) = 0;

			virtual void handleIncomingRequests() = 0;

			virtual ~MutexServer() {}
	};

}
#endif
