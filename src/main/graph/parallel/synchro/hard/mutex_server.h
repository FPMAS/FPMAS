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

	using FPMAS::api::graph::parallel::synchro::hard::Request;
	using FPMAS::api::graph::parallel::synchro::hard::RequestType;

	template<typename T, template<typename> class Mpi>
		class MutexServer :
			public FPMAS::api::graph::parallel::synchro::hard::MutexServer<T> {
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T>
					mutex_server_base;
				typedef FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>
					hard_sync_mutex;
				typedef FPMAS::api::communication::MpiCommunicator comm_t;

				private:
				Epoch epoch = Epoch::EVEN;
				std::unordered_map<DistributedId, hard_sync_mutex*> mutexMap;
				comm_t& comm;
				Mpi<DistributedId> idMpi {comm};
				Mpi<T> dataMpi {comm};
				Mpi<DataUpdatePack<T>> dataUpdateMpi {comm};

				void handleIncomingReadAcquireLock();
				void handleRead(DistributedId id, int source);
				void respondToRead(DistributedId id, int source);

				void handleAcquire(DistributedId id, int source);
				void respondToAcquire(DistributedId id, int source);

				void respondToRequests(hard_sync_mutex*);
				void handleRelease(DataUpdatePack<T>& update);

				void handleLock(DistributedId id, int source);
				void respondToLock(DistributedId id, int source);

				void handleUnlock(DistributedId id);

				bool respondToRequests(hard_sync_mutex*, const Request& requestToWait);
				bool handleIncomingRequests(const Request& requestToWait);
				bool handleRelease(DataUpdatePack<T>& update, const Request& requestToWait);
				bool handleUnlock(DistributedId id, const Request& requestToWait);

				public:
				MutexServer(comm_t& comm) : comm(comm) {}

				const Mpi<DistributedId>& getIdMpi() const {return idMpi;}
				const Mpi<T>& getDataMpi() const {return dataMpi;}
				const Mpi<DataUpdatePack<T>>& getDataUpdateMpi() const {return dataUpdateMpi;}

				void setEpoch(Epoch e) override {this->epoch = e;}
				Epoch getEpoch() const override {return this->epoch;}

				void manage(DistributedId id, hard_sync_mutex* mutex) override {
					mutexMap.insert({id, mutex});
				}
				void remove(DistributedId id) override {
					mutexMap.erase(id);
				}

				const std::unordered_map<DistributedId, hard_sync_mutex*>& getManagedMutexes() const {
					return mutexMap;
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
			DistributedId id = idMpi.recv(&req_status);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "read request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleRead(id, req_status.MPI_SOURCE);
		}

		// Check acquire request
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::ACQUIRE, &req_status)) {
			DistributedId id = idMpi.recv(&req_status);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "acquire request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleAcquire(id, req_status.MPI_SOURCE);
		}
		
		// Check lock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LOCK, &req_status)) {
			DistributedId id = idMpi.recv(&req_status);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "lock request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleLock(id, req_status.MPI_SOURCE);
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

		// Check release
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::RELEASE, &req_status)) {
			DataUpdatePack<T> update = dataUpdateMpi.recv(&req_status);
			//FPMAS_LOGD(this->comm.getRank(), "RECV", "released from %i : %s", req_status.MPI_SOURCE, data.c_str());
			this->handleRelease(update);
		}
	
		// Check unlock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK, &req_status)) {
			DistributedId id = idMpi.recv(&req_status);

			this->handleUnlock(id);
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
		auto* mutex = mutexMap.at(id);
		if(mutex->locked()) {
			mutex->pushRequest(Request(id, source, RequestType::READ));
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
		// Perform the response
		const T& data = this->mutexMap.at(id)->data();
		dataMpi.send(data, source, epoch | Tag::READ_RESPONSE);
	}

	/*
	 * Handles an acquire request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond if the resource immediately is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleAcquire(DistributedId id, int source) {
		auto* mutex = mutexMap.at(id);
		if(mutex->locked()) {
			mutex->pushRequest(Request(id, source, RequestType::ACQUIRE));
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
		auto* mutex = mutexMap.at(id);
		this->mutex_server_base::lock(mutex);
		const T& data = mutex->data();
		dataMpi.send(data, source, epoch | Tag::ACQUIRE_RESPONSE);
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::respondToRequests(hard_sync_mutex* mutex) {
		std::queue<Request> requests = mutex->requestsToProcess();
		while(!requests.empty()) {
			Request request = requests.front();
			if(request.source != Request::LOCAL) {
				switch(request.type) {
					case RequestType::READ :
						respondToRead(request.id, request.source);
						break;
					case RequestType::LOCK :
						respondToLock(request.id, request.source);
						break;
					case RequestType::ACQUIRE :
						respondToAcquire(request.id, request.source);
				}
				requests.pop();
			}
		}
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleRelease(DataUpdatePack<T>& update) {
		auto* mutex = mutexMap.at(update.id);
		this->mutex_server_base::unlock(mutex);
		mutex->data() = update.updatedData;

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
		auto* mutex = mutexMap.at(id);
		if(mutex->locked()) {
			mutex->pushRequest(Request(id, source, RequestType::LOCK));
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
		auto* mutex = mutexMap.at(id);
		this->mutex_server_base::lock(mutex);
		comm.send(source, epoch | Tag::LOCK_RESPONSE);
	}

	template<typename T, template<typename> class Mpi>
	void MutexServer<T, Mpi>::handleUnlock(DistributedId id) {
		auto* mutex = mutexMap.at(id);
		this->mutex_server_base::unlock(mutex);

		respondToRequests(mutex);
	}


	/* Wait variants */

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::respondToRequests(hard_sync_mutex* mutex, const Request& requestToWait) {
		bool requestToWaitProcessed = false;
		std::queue<Request> requests = mutex->requestsToProcess();
		while(!requests.empty()) {
			Request request = requests.front();
			if(request.source != -1) {
				switch(request.type) {
					case RequestType::READ :
						respondToRead(request.id, request.source);
						break;
					case RequestType::LOCK :
						respondToLock(request.id, request.source);
						break;
					case RequestType::ACQUIRE :
						respondToAcquire(request.id, request.source);
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
	bool MutexServer<T, Mpi>::handleRelease(DataUpdatePack<T>& update, const Request& requestToWait) {
		auto* mutex = mutexMap.at(update.id);
		this->mutex_server_base::unlock(mutex);
		mutex->data() = update.updatedData;

		return respondToRequests(mutex, requestToWait);
	}

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::handleUnlock(DistributedId id, const Request& requestToWait) {
		auto* mutex = mutexMap.at(id);
		this->mutex_server_base::unlock(mutex);

		return respondToRequests(mutex, requestToWait);
	}

	template<typename T, template<typename> class Mpi>
	bool MutexServer<T, Mpi>::handleIncomingRequests(const Request& requestToWait) {
		handleIncomingReadAcquireLock();

		MPI_Status req_status;
		// Check release
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::RELEASE, &req_status)) {
			DataUpdatePack<T> update = dataUpdateMpi.recv(&req_status);
			//FPMAS_LOGD(this->comm.getRank(), "RECV", "released from %i : %s", req_status.MPI_SOURCE, data.c_str());
			if(this->handleRelease(update, requestToWait)){
				return true;
			}
		}
	
		// Check unlock
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK, &req_status)) {
			DistributedId id = idMpi.recv(&req_status);

			if(this->handleUnlock(id, requestToWait)) {
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
		auto* mutex = mutexMap.at(id);
		return respondToRequests(mutex);
	}
}
#endif
