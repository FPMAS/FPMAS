#ifndef MUTEX_SERVER_H
#define MUTEX_SERVER_H

#include "utils/macros.h"
#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/hard/enums.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"
#include "data_update_pack.h"

namespace FPMAS::graph::parallel::synchro::hard {
	using FPMAS::api::graph::parallel::synchro::hard::Epoch;
	using FPMAS::api::graph::parallel::synchro::hard::Tag;

	using FPMAS::api::graph::parallel::synchro::hard::MutexRequestType;

	template<typename T, template<typename> class Mpi>
		class MutexServer :
			public FPMAS::api::graph::parallel::synchro::hard::MutexServer<T> {
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T>
					MutexServerBase;
				typedef FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>
					HardSyncMutex;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexRequest
					Request;
				typedef FPMAS::api::communication::MpiCommunicator MpiComm;

				private:
				Epoch epoch = Epoch::EVEN;
				std::unordered_map<DistributedId, HardSyncMutex*> mutex_map;
				MpiComm& comm;
				Mpi<DistributedId> id_mpi {comm};
				Mpi<T> data_mpi {comm};
				Mpi<DataUpdatePack<T>> data_update_mpi {comm};

				void handleIncomingReadAcquireLock();

				void handleRead(DistributedId id, int source);
				void respondToRead(DistributedId id, int source);

				void handleAcquire(DistributedId id, int source);
				void respondToAcquire(DistributedId id, int source);
				void handleReleaseAcquire(DataUpdatePack<T>& update);

				void respondToRequests(HardSyncMutex*);

				void handleLock(DistributedId id, int source);
				void respondToLock(DistributedId id, int source);
				void handleUnlock(DistributedId id);

				void handleLockShared(DistributedId id, int source);
				void respondToLockShared(DistributedId id, int source);
				void handleUnlockShared(DistributedId id);

				bool respondToRequests(HardSyncMutex*, const Request& requestToWait);
				bool handleIncomingRequests(const Request& requestToWait);
				bool handleReleaseAcquire(DataUpdatePack<T>& update, const Request& requestToWait);
				bool handleUnlock(DistributedId id, const Request& requestToWait);
				bool handleUnlockShared(DistributedId id, const Request& requestToWait);

				public:
				MutexServer(MpiComm& comm) : comm(comm) {}

				const Mpi<DistributedId>& getIdMpi() const {return id_mpi;}
				const Mpi<T>& getDataMpi() const {return data_mpi;}
				const Mpi<DataUpdatePack<T>>& getDataUpdateMpi() const {return data_update_mpi;}

				void setEpoch(Epoch e) override {this->epoch = e;}
				Epoch getEpoch() const override {return this->epoch;}

				void manage(DistributedId id, HardSyncMutex* mutex) override {
					mutex_map.insert({id, mutex});
				}
				void remove(DistributedId id) override {
					mutex_map.erase(id);
				}

				const std::unordered_map<DistributedId, HardSyncMutex*>& getManagedMutexes() const {
					return mutex_map;
				}

				void handleIncomingRequests() override;

				void wait(const Request&) override;
				void notify(DistributedId) override;
			};

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleIncomingReadAcquireLock() {
		MPI_Status req_status;

		// Check read request
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::READ, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "read request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleRead(id, req_status.MPI_SOURCE);
		}

		// Check acquire request
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::ACQUIRE, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "acquire request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleAcquire(id, req_status.MPI_SOURCE);
		}
		
		// Check lock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LOCK, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "lock request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleLock(id, req_status.MPI_SOURCE);
		}

		// Check shared lock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LOCK_SHARED, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "shared lock request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleLockShared(id, req_status.MPI_SOURCE);
		}
	}
	/**
	 * Performs a reception cycle to handle.
	 *
	 * This function should not be used by the user, but is left public because it
	 * might be useful for unit testing.
	 *
	 * A call to this function will receive and handle at most one of each of the
	 * following requests types :
	 * - read request
	 * - acquire request
	 * - given back data
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleIncomingRequests() {
		MPI_Status req_status;
		handleIncomingReadAcquireLock();

		// Check release acquire
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::RELEASE_ACQUIRE, &req_status)) {
			DataUpdatePack<T> update = data_update_mpi.recv(&req_status);

			this->handleReleaseAcquire(update);
		}
	
		// Check unlock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);

			this->handleUnlock(id);
		}

		// Check shared unlock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK_SHARED, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);

			this->handleUnlockShared(id);
		}
	}

	/*
	 * Handles a read request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond immediately if the resource is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleRead(DistributedId id, int source) {
		auto* mutex = mutex_map.at(id);
		if(mutex->locked()) {
			mutex->pushRequest(Request(id, source, MutexRequestType::READ));
		} else {
			respondToRead(id, source);
		}
	}

	/*
	 * Sends a read response to the source proc, reading data using the
	 * resourceManager.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::respondToRead(DistributedId id, int source) {
		auto* mutex = mutex_map.at(id);
		this->MutexServerBase::lockShared(mutex);
		// Perform the response
		const T& data = this->mutex_map.at(id)->data();
		data_mpi.send(data, source, epoch | Tag::READ_RESPONSE);
	}

	/*
	 * Handles an acquire request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond if the resource immediately is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleAcquire(DistributedId id, int source) {
		auto* mutex = mutex_map.at(id);
		if(mutex->locked() || mutex->lockedShared() > 0) {
			mutex->pushRequest(Request(id, source, MutexRequestType::ACQUIRE));
		} else {
			respondToAcquire(id, source);
		}
	}


	/*
	 * Sends an acquire response to the source proc, reading data using the
	 * resourceManager.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::respondToAcquire(DistributedId id, int source) {
		auto* mutex = mutex_map.at(id);
		this->MutexServerBase::lock(mutex);
		const T& data = mutex->data();
		data_mpi.send(data, source, epoch | Tag::ACQUIRE_RESPONSE);
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::respondToRequests(HardSyncMutex* mutex) {
		std::queue<Request> requests = mutex->requestsToProcess();
		while(!requests.empty()) {
			Request request = requests.front();
			if(request.source != Request::LOCAL) {
				switch(request.type) {
					case MutexRequestType::READ :
						respondToRead(request.id, request.source);
						break;
					case MutexRequestType::LOCK :
						respondToLock(request.id, request.source);
						break;
					case MutexRequestType::ACQUIRE :
						respondToAcquire(request.id, request.source);
						break;
					case MutexRequestType::LOCK_SHARED :
						respondToLockShared(request.id, request.source);
				}
				requests.pop();
			}
		}
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleReleaseAcquire(DataUpdatePack<T>& update) {
		auto* mutex = mutex_map.at(update.id);
		this->MutexServerBase::unlock(mutex);
		mutex->data() = update.updated_data;

		respondToRequests(mutex);
	}

	/*
	 * Handles a lock request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond if the resource immediately is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleLock(DistributedId id, int source) {
		auto* mutex = mutex_map.at(id);
		if(mutex->locked() || mutex->lockedShared() > 0) {
			mutex->pushRequest(Request(id, source, MutexRequestType::LOCK));
		} else {
			respondToLock(id, source);
		}
	}


	/*
	 * Sends an acquire response to the source proc, reading data using the
	 * resourceManager.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::respondToLock(DistributedId id, int source) {
		auto* mutex = mutex_map.at(id);
		this->MutexServerBase::lock(mutex);
		comm.send(source, epoch | Tag::LOCK_RESPONSE);
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleUnlock(DistributedId id) {
		auto* mutex = mutex_map.at(id);
		this->MutexServerBase::unlock(mutex);

		respondToRequests(mutex);
	}

	template<typename T, template<typename> class Mpi>
		void MutexServer<T, Mpi>::handleLockShared(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			if(mutex->locked()) {
				mutex->pushRequest(Request(id, source, MutexRequestType::LOCK_SHARED));
			} else {
				respondToLockShared(id, source);
			}
		}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::respondToLockShared(DistributedId id, int source) {
		auto* mutex = mutex_map.at(id);
		this->MutexServerBase::lockShared(mutex);
		comm.send(source, epoch | Tag::LOCK_SHARED_RESPONSE);
	}

	template<typename T, template<typename> class Mpi>
		void MutexServer<T, Mpi>::handleUnlockShared(DistributedId id) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::unlockShared(mutex);

			// No requests will be processed if they is still at least one
			// shared lock. See mutex::requestsToProcess.
			respondToRequests(mutex);
		}


	/* Wait variants */

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::respondToRequests(HardSyncMutex* mutex, const Request& requestToWait) {
		bool requestToWaitProcessed = false;
		std::queue<Request> requests = mutex->requestsToProcess();
		while(!requests.empty()) {
			Request request = requests.front();
			if(request.source != -1) {
				switch(request.type) {
					case MutexRequestType::READ :
						respondToRead(request.id, request.source);
						break;
					case MutexRequestType::LOCK :
						respondToLock(request.id, request.source);
						break;
					case MutexRequestType::ACQUIRE :
						respondToAcquire(request.id, request.source);
						break;
					case MutexRequestType::LOCK_SHARED :
						respondToLockShared(request.id, request.source);
				}
				requests.pop();
			} else {
				if(request == requestToWait) {
					requestToWaitProcessed = true;
				}
			}
		}
		return requestToWaitProcessed;
	}

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::handleReleaseAcquire(DataUpdatePack<T>& update, const Request& requestToWait) {
		auto* mutex = mutex_map.at(update.id);
		this->MutexServerBase::unlock(mutex);
		mutex->data() = update.updated_data;

		return respondToRequests(mutex, requestToWait);
	}

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::handleUnlock(DistributedId id, const Request& requestToWait) {
		auto* mutex = mutex_map.at(id);
		this->MutexServerBase::unlock(mutex);

		return respondToRequests(mutex, requestToWait);
	}

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::handleUnlockShared(DistributedId id, const Request& requestToWait) {
		auto* mutex = mutex_map.at(id);
		this->MutexServerBase::unlockShared(mutex);

		return respondToRequests(mutex, requestToWait);
	}

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::handleIncomingRequests(const Request& requestToWait) {
		handleIncomingReadAcquireLock();

		MPI_Status req_status;

		// Check release acquire
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::RELEASE_ACQUIRE, &req_status)) {
			DataUpdatePack<T> update = data_update_mpi.recv(&req_status);
			//FPMAS_LOGD(this->comm.getRank(), "RECV", "released from %i : %s", req_status.MPI_SOURCE, data.c_str());
			if(this->handleReleaseAcquire(update, requestToWait)){
				return true;
			}
		}
	
		// Check unlock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);

			if(this->handleUnlock(id, requestToWait)) {
				return true;
			}
		}

		// Check shared unlock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK_SHARED, &req_status)) {
			DistributedId id = id_mpi.recv(&req_status);

			if(this->handleUnlockShared(id, requestToWait)) {
				return true;
			}
		}
		return false;
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::wait(const Request& requestToWait) {
		bool requestProcessed = false;
		while(!requestProcessed) {
			requestProcessed = handleIncomingRequests(requestToWait);
		}
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::notify(DistributedId id) {
		auto* mutex = mutex_map.at(id);
		return respondToRequests(mutex);
	}
}
#endif
