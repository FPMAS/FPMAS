#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include "enums.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel::synchro::hard {
	enum class RequestType {
		READ, LOCK, ACQUIRE
	};
	struct Request {
		static const int LOCAL = -1;
		DistributedId id;
		int source;
		RequestType type;

		Request(DistributedId id, int source, RequestType type)
			: id(id), source(source), type(type) {}

		bool operator==(const Request& other) const {
			return
				(this->type == other.type) &&
				(this->id == other.id) &&
				(this->source == other.source);
		}
	};

	template<typename T>
		class MutexClient {
			public:
				virtual T read(DistributedId, int location) = 0;
				virtual T acquire(DistributedId, int location) = 0;
				virtual void release(DistributedId, const T&, int location) = 0;

				virtual void lock(DistributedId, int location) = 0;
				virtual void unlock(DistributedId, int location) = 0;

				virtual ~MutexClient() {}
		};

	template<typename T> class HardSyncMutex;

	class Server {
		public:
			virtual void setEpoch(Epoch) = 0;
			virtual Epoch getEpoch() const = 0;

			virtual void handleIncomingRequests() = 0;
	};

	template<typename T>
		class MutexServer : public virtual Server {
			friend HardSyncMutex<T>;
			protected:
			void lock(HardSyncMutex<T>* mutex) {mutex->_lock();}
			void unlock(HardSyncMutex<T>* mutex) {mutex->_unlock();}
			public:
			virtual void manage(DistributedId id, HardSyncMutex<T>*) = 0;
			virtual void remove(DistributedId id) = 0;

			virtual void wait(const Request& req) = 0;
			virtual void notify(DistributedId) = 0;


			virtual ~MutexServer() {}
		};

	class TerminationAlgorithm {
		public:
			virtual void terminate(Server& server) = 0;
	};


}
#endif
