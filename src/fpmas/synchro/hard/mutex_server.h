#ifndef FPMAS_MUTEX_SERVER_H
#define FPMAS_MUTEX_SERVER_H

/** \file src/fpmas/synchro/hard/mutex_server.h
 * MutexServer implementation.
 */

#include "fpmas/utils/macros.h"
#include "fpmas/api/communication/communication.h"
#include "fpmas/synchro/hard/api/hard_sync_mode.h"
#include "../data_update_pack.h"
#include "fpmas/utils/log.h"
#include "server_pack.h"

namespace fpmas { namespace synchro { namespace hard {
	using api::Epoch;
	using api::Tag;

	using api::MutexRequestType;

	/**
	 * api::MutexServer implementation.
	 */
	template<typename T>
		class MutexServer :
			public api::MutexServer<T> {
				typedef api::MutexServer<T> MutexServerBase;
				typedef api::HardSyncMutex<T> HardSyncMutex;
				typedef api::MutexRequest Request;
				typedef fpmas::api::communication::MpiCommunicator MpiComm;
				typedef fpmas::api::communication::TypedMpi<DistributedId> IdMpi;
				typedef fpmas::api::communication::TypedMpi<T> DataMpi;
				typedef fpmas::api::communication::TypedMpi<DataUpdatePack<T>> DataUpdateMpi;

				private:
				Epoch epoch = Epoch::EVEN;
				std::unordered_map<DistributedId, HardSyncMutex*> mutex_map;
				MpiComm& comm;
				IdMpi& id_mpi;
				DataMpi& data_mpi;
				DataUpdateMpi& data_update_mpi;
				ServerPackBase& server_pack;

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

				bool respondToRequests(HardSyncMutex*, const Request& request_to_wait);
				bool handleIncomingRequests(const Request& request_to_wait);
				bool handleReleaseAcquire(DataUpdatePack<T>& update, const Request& request_to_wait);
				bool handleUnlock(DistributedId id, const Request& request_to_wait);
				bool handleUnlockShared(DistributedId id, const Request& request_to_wait);

				public:
				/**
				 * MutexServer constructor.
				 *
				 * @param comm MPI communicator
				 * @param id_mpi IdMpi instance
				 * @param data_mpi DataMpi instance
				 * @param data_update_mpi DataUpdateMpi instance
				 * @param server_pack the ServerPack instance used for
				 * HardDataSync synchronization.
				 */
				MutexServer(
						MpiComm& comm, IdMpi& id_mpi, DataMpi& data_mpi,
						DataUpdateMpi& data_update_mpi,
						ServerPackBase& server_pack
						) :
					comm(comm), id_mpi(id_mpi), data_mpi(data_mpi),
					data_update_mpi(data_update_mpi),
					server_pack(server_pack) {
					}

				void setEpoch(api::Epoch e) override {this->epoch = e;}
				Epoch getEpoch() const override {return this->epoch;}

				void manage(DistributedId id, HardSyncMutex* mutex) override {
					mutex_map[id] = mutex;
				}
				void remove(DistributedId id) override {
					mutex_map.erase(id);
				}

				/**
				 * Returns a map of mutexes managed by this MutexServer.
				 *
				 * @return managed mutex map
				 */
				const std::unordered_map<DistributedId, HardSyncMutex*>& getManagedMutexes() const {
					return mutex_map;
				}

				void handleIncomingRequests() override;

				void wait(const Request&) override;
				void notify(DistributedId) override;
			};

	template<typename T>
		void MutexServer<T>::handleIncomingReadAcquireLock() {
			fpmas::api::communication::Status status;

			// Check read request
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::READ, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGD(this->comm.getRank(), "MUTEX_SERVER", "receive read request %s from %i", FPMAS_C_STR(id), status.source);
				this->handleRead(id, status.source);
			}

			// Check acquire request
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::ACQUIRE, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGD(this->comm.getRank(), "MUTEX_SERVER", "receive acquire request %s from %i", FPMAS_C_STR(id), status.source);
				this->handleAcquire(id, status.source);
			}

			// Check lock
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LOCK, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGD(this->comm.getRank(), "MUTEX_SERVER", "receive lock request %s from %i", FPMAS_C_STR(id), status.source);
				this->handleLock(id, status.source);
			}

			// Check shared lock
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::LOCK_SHARED, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGD(this->comm.getRank(), "MUTEX_SERVER", "receive shared lock request %s from %i", FPMAS_C_STR(id), status.source);
				this->handleLockShared(id, status.source);
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
	template<typename T>
		void MutexServer<T>::handleIncomingRequests() {
			fpmas::api::communication::Status status;
			handleIncomingReadAcquireLock();

			// Check release acquire
			if(data_update_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::RELEASE_ACQUIRE, status)) {
				DataUpdatePack<T> update = data_update_mpi.recv(status.source, status.tag);
				FPMAS_LOGV(this->comm.getRank(), "MUTEX_SERVER", "receive release acquire %s from %i",
						FPMAS_C_STR(update.id), status.source);

				this->handleReleaseAcquire(update);
			}

			// Check unlock
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGV(this->comm.getRank(), "MUTEX_SERVER", "receive unlock %s from %i",
						FPMAS_C_STR(id), status.source);

				this->handleUnlock(id);
			}

			// Check shared unlock
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK_SHARED, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGV(this->comm.getRank(), "MUTEX_SERVER", "receive unlock shared %s from %i",
						FPMAS_C_STR(id), status.source);

				this->handleUnlockShared(id);
			}
		}

	/*
	 * Handles a read request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond immediately if the resource is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T>
		void MutexServer<T>::handleRead(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			if(mutex->locked()) {
				FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "Enqueueing READ request of node %s for %i",
						FPMAS_C_STR(id), source);
				mutex->pushRequest(Request(id, source, MutexRequestType::READ));
			} else {
				respondToRead(id, source);
			}
		}

	/*
	 * Sends a read response to the source proc, reading data using the
	 * resourceManager.
	 */
	template<typename T>
		void MutexServer<T>::respondToRead(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::lockShared(mutex);
			// Perform the response
			const T& data = this->mutex_map.at(id)->data();
			server_pack.pendingRequests().emplace_back(
					fpmas::api::communication::Request());
			data_mpi.Isend(
					data, source, epoch | Tag::READ_RESPONSE,
					server_pack.pendingRequests().back());
		}

	/*
	 * Handles an acquire request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond if the resource immediately is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T>
		void MutexServer<T>::handleAcquire(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			if(mutex->locked() || mutex->sharedLockCount() > 0) {
				FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "Enqueueing ACQUIRE request of node %s for %i",
						FPMAS_C_STR(id), source);
				mutex->pushRequest(Request(id, source, MutexRequestType::ACQUIRE));
			} else {
				respondToAcquire(id, source);
			}
		}


	/*
	 * Sends an acquire response to the source proc, reading data using the
	 * resourceManager.
	 */
	template<typename T>
		void MutexServer<T>::respondToAcquire(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::lock(mutex);
			const T& data = mutex->data();
			server_pack.pendingRequests().emplace_back(
					fpmas::api::communication::Request());
			data_mpi.Isend(
					data, source, epoch | Tag::ACQUIRE_RESPONSE,
					server_pack.pendingRequests().back()
					);
		}

	template<typename T>
		void MutexServer<T>::respondToRequests(HardSyncMutex* mutex) {
			FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "Unqueueing requests...", "");
			std::queue<Request> requests = mutex->requestsToProcess();
			while(!requests.empty()) {
				Request request = requests.front();
				switch(request.type) {
					case MutexRequestType::READ :
						FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->READ(%s)",
								request.source, FPMAS_C_STR(request.id));
						respondToRead(request.id, request.source);
						break;
					case MutexRequestType::LOCK :
						FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->LOCK(%s)",
								request.source, FPMAS_C_STR(request.id));
						respondToLock(request.id, request.source);
						break;
					case MutexRequestType::ACQUIRE :
						FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->ACQUIRE(%s)",
								request.source, FPMAS_C_STR(request.id));
						respondToAcquire(request.id, request.source);
						break;
					case MutexRequestType::LOCK_SHARED :
						FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->LOCK_SHARED(%s)",
								request.source, FPMAS_C_STR(request.id));
						respondToLockShared(request.id, request.source);
				}
				requests.pop();
			}
		}

	template<typename T>
		void MutexServer<T>::handleReleaseAcquire(DataUpdatePack<T>& update) {
			auto* mutex = mutex_map.at(update.id);
			this->MutexServerBase::unlock(mutex);
			mutex->data() = std::move(update.updated_data);

			respondToRequests(mutex);
		}

	/*
	 * Handles a lock request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond if the resource immediately is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T>
		void MutexServer<T>::handleLock(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			if(mutex->locked() || mutex->sharedLockCount() > 0) {
				FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "Enqueueing LOCK request of node %s for %i",
						FPMAS_C_STR(id), source);
				mutex->pushRequest(Request(id, source, MutexRequestType::LOCK));
			} else {
				respondToLock(id, source);
			}
		}


	/*
	 * Sends an acquire response to the source proc, reading data using the
	 * resourceManager.
	 */
	template<typename T>
		void MutexServer<T>::respondToLock(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::lock(mutex);

			server_pack.pendingRequests().emplace_back(
					fpmas::api::communication::Request());
			comm.Isend(source, epoch | Tag::LOCK_RESPONSE,
					server_pack.pendingRequests().back());
		}

	template<typename T>
		void MutexServer<T>::handleUnlock(DistributedId id) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::unlock(mutex);

			respondToRequests(mutex);
		}

	template<typename T>
		void MutexServer<T>::handleLockShared(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			if(mutex->locked()) {
				FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "Enqueueing LOCK_SHARED request of node %s for %i",
						FPMAS_C_STR(id), source);
				mutex->pushRequest(Request(id, source, MutexRequestType::LOCK_SHARED));
			} else {
				respondToLockShared(id, source);
			}
		}

	template<typename T>
		void MutexServer<T>::respondToLockShared(DistributedId id, int source) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::lockShared(mutex);

			server_pack.pendingRequests().emplace_back(
					fpmas::api::communication::Request());
			comm.Isend(source, epoch | Tag::LOCK_SHARED_RESPONSE,
					server_pack.pendingRequests().back());
		}

	template<typename T>
		void MutexServer<T>::handleUnlockShared(DistributedId id) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::unlockShared(mutex);

			// No requests will be processed if they is still at least one
			// shared lock. See mutex::requestsToProcess.
			respondToRequests(mutex);
		}


	/* Wait variants */

	template<typename T>
		bool MutexServer<T>::respondToRequests(HardSyncMutex* mutex, const Request& request_to_wait) {
			FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "Unqueueing requests...", "");
			std::queue<Request> requests = mutex->requestsToProcess();
			while(!requests.empty()) {
				Request request = requests.front();
				if(request.source != Request::LOCAL) {
					switch(request.type) {
						case MutexRequestType::READ :
							FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->READ(%s)",
									request.source, FPMAS_C_STR(request.id));
							respondToRead(request.id, request.source);
							break;
						case MutexRequestType::LOCK :
							FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->LOCK(%s)",
									request.source, FPMAS_C_STR(request.id));
							respondToLock(request.id, request.source);
							break;
						case MutexRequestType::ACQUIRE :
							FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->ACQUIRE(%s)",
									request.source, FPMAS_C_STR(request.id));
							respondToAcquire(request.id, request.source);
							break;
						case MutexRequestType::LOCK_SHARED :
							FPMAS_LOGV(comm.getRank(), "MUTEX_SERVER", "%i->LOCK_SHARED(%s)",
									request.source, FPMAS_C_STR(request.id));
							respondToLockShared(request.id, request.source);
					}
					requests.pop();
				} else {
					if(request == request_to_wait) {
						return true;
					}
				}
			}
			return false;
		}

	template<typename T>
		bool MutexServer<T>::handleReleaseAcquire(DataUpdatePack<T>& update, const Request& request_to_wait) {
			auto* mutex = mutex_map.at(update.id);
			this->MutexServerBase::unlock(mutex);
			mutex->data() = std::move(update.updated_data);

			return respondToRequests(mutex, request_to_wait);
		}

	template<typename T>
		bool MutexServer<T>::handleUnlock(DistributedId id, const Request& request_to_wait) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::unlock(mutex);

			return respondToRequests(mutex, request_to_wait);
		}

	template<typename T>
		bool MutexServer<T>::handleUnlockShared(DistributedId id, const Request& request_to_wait) {
			auto* mutex = mutex_map.at(id);
			this->MutexServerBase::unlockShared(mutex);

			return respondToRequests(mutex, request_to_wait);
		}

	template<typename T>
		bool MutexServer<T>::handleIncomingRequests(const Request& request_to_wait) {
			handleIncomingReadAcquireLock();

			fpmas::api::communication::Status status;

			// Check release acquire
			if(data_update_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::RELEASE_ACQUIRE, status)) {
				DataUpdatePack<T> update = data_update_mpi.recv(status.source, status.tag);
				FPMAS_LOGV(this->comm.getRank(), "MUTEX_SERVER", "receive release acquire %s from %i",
						FPMAS_C_STR(update.id), status.source);
				if(this->handleReleaseAcquire(update, request_to_wait)){
					return true;
				}
			}

			// Check unlock
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);
				FPMAS_LOGV(this->comm.getRank(), "MUTEX_SERVER", "receive unlock %s from %i",
						FPMAS_C_STR(id), status.source);
				if(this->handleUnlock(id, request_to_wait)) {
					return true;
				}
			}

			// Check shared unlock
			if(id_mpi.Iprobe(MPI_ANY_SOURCE, epoch | Tag::UNLOCK_SHARED, status)) {
				DistributedId id = id_mpi.recv(status.source, status.tag);

				FPMAS_LOGV(this->comm.getRank(), "MUTEX_SERVER", "receive unlock shared %s from %i",
						FPMAS_C_STR(id), status.source);
				if(this->handleUnlockShared(id, request_to_wait)) {
					return true;
				}
			}
			return false;
		}

	template<typename T>
		void MutexServer<T>::wait(const Request& request_to_wait) {
			FPMAS_LOGD(comm.getRank(), "MUTEX_SERVER",
					"Waiting for local request to node %s to complete...",
					FPMAS_C_STR(request_to_wait.id));
			bool request_processed = false;
			while(!request_processed) {
				request_processed = handleIncomingRequests(request_to_wait);
				server_pack.linkServer().handleIncomingRequests();
			}
			FPMAS_LOGD(comm.getRank(), "MUTEX_SERVER",
					"Handling local request to node %s.",
					FPMAS_C_STR(request_to_wait.id));
		}

	template<typename T>
		void MutexServer<T>::notify(DistributedId id) {
			FPMAS_LOGD(comm.getRank(), "MUTEX_SERVER",
					"Notifying released node %s", FPMAS_C_STR(id));
			auto* mutex = mutex_map.at(id);
			return respondToRequests(mutex);
		}
}}}
#endif
