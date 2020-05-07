#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include "enums.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel::synchro::hard {
	struct MutexRequest {
		static const int LOCAL = -1;
		DistributedId id;
		int source;
		MutexRequestType type;

		MutexRequest(DistributedId id, int source, MutexRequestType type)
			: id(id), source(source), type(type) {}

		bool operator==(const MutexRequest& other) const {
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

	class Server {
		public:
			virtual void setEpoch(Epoch) = 0;
			virtual Epoch getEpoch() const = 0;

			virtual void handleIncomingRequests() = 0;
	};

	template<typename T> class HardSyncMutex;
	template<typename T>
		class MutexServer : public virtual Server {
			friend HardSyncMutex<T>;
			protected:
			void lock(HardSyncMutex<T>* mutex) {mutex->_lock();}
			void unlock(HardSyncMutex<T>* mutex) {mutex->_unlock();}
			public:
			virtual void manage(DistributedId id, HardSyncMutex<T>*) = 0;
			virtual void remove(DistributedId id) = 0;

			virtual void wait(const MutexRequest& req) = 0;
			virtual void notify(DistributedId) = 0;


			virtual ~MutexServer() {}
		};

	template<typename Arc>
		class LinkClient {
			public:
				virtual void link(const Arc*) = 0;
				virtual void unlink(const Arc*) = 0;

				virtual ~LinkClient() {};
		};

	class LinkServer : public virtual Server {
	};

	class TerminationAlgorithm {
		public:
			virtual void terminate(Server& server) = 0;

			virtual ~TerminationAlgorithm() {};
	};


}
#endif
