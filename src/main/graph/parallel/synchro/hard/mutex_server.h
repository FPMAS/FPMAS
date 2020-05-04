#ifndef MUTEX_SERVER_H
#define MUTEX_SERVER_H

#include "utils/macros.h"
#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/hard/enums.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {
	using FPMAS::api::graph::parallel::synchro::hard::Epoch;
	using FPMAS::api::graph::parallel::synchro::hard::Tag;

	template<typename T>
		class MutexServer :
			public FPMAS::api::graph::parallel::synchro::hard::MutexServer<T> {
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T>
					mutex_server_base;
				typedef FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>
					hard_sync_mutex;
				typedef FPMAS::api::graph::parallel::synchro::hard::Request
					request_t;
				typedef FPMAS::api::communication::Mpi<T> mpi_t;
				typedef FPMAS::api::communication::MpiCommunicator comm_t;

				private:
				Epoch epoch;
				std::unordered_map<DistributedId, hard_sync_mutex*> mutexMap;
				mpi_t& mpi;
				comm_t& comm;

				void handleRead(DistributedId id, int destination);
				void respondToRead(DistributedId id, int destination);
				void handleAcquire(DistributedId id, int destination);
				void respondToAcquire(DistributedId id, int destination);

				public:
				MutexServer(mpi_t& mpi, comm_t& comm) : mpi(mpi), comm(comm) {}
				void manage(DistributedId id, hard_sync_mutex* mutex) override {
					mutexMap.insert({id, mutex});
				}
				void remove(DistributedId id) override {
					mutexMap.erase(id);
				}
				void handleIncomingRequests() override;

				void wait(const request_t& req) override {};
				void notify(DistributedId) override {};
			};

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
		MPI_Status req_status;

		// Check read request
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::READ, &req_status) > 0) {
			DistributedId id;
			comm.recv(&req_status, id);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "read request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleRead(id, req_status.MPI_SOURCE);
		}

		// Check acquire request
		if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::ACQUIRE, &req_status) > 0) {
			DistributedId id;
			comm.recv(&req_status, id);
			FPMAS_LOGD(this->comm.getRank(), "RECV", "acquire request %s from %i", ID_C_STR(id), req_status.MPI_SOURCE);
			this->handleAcquire(id, req_status.MPI_SOURCE);
		}
/*
 *
 *        // Check acquire give back
 *        if(comm.Iprobe(MPI_ANY_SOURCE, epoch | Tag::ACQUIRE_GIVE_BACK, &req_status) > 0) {
 *            std::string data;
 *            comm.recv(&req_status, data);
 *            FPMAS_LOGD(this->comm.getRank(), "RECV", "given back from %i : %s", req_status.MPI_SOURCE, data.c_str());
 *            this->handleGiveBack(data);
 *        }
 */
	}

	/*
	 * Handles a read request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond immediately if the resource is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T>
	void MutexServer<T>::handleRead(DistributedId id, int destination) {
		auto* mutex = mutexMap[id];
		if(mutex->locked()) {
			mutex->readRequests().push(request_t(id, destination));
		} else {
			respondToRead(id, destination);
		}
	}

	/*
	 * Sends a read response to the destination proc, reading data using the
	 * resourceManager.
	 */
	template<typename T>
	void MutexServer<T>::respondToRead(DistributedId id, int destination) {
		// Perform the response
		const T& data = this->mutexMap[id]->data();
		mpi.send(data, destination, epoch | Tag::READ_RESPONSE);
	}

	/*
	 * Handles an acquire request.
	 * The request is transmitted to the corresponding ReaderWriter instance, that
	 * will respond if the resource immediately is available or put the request in an waiting
	 * queue otherwise.
	 */
	template<typename T>
	void MutexServer<T>::handleAcquire(DistributedId id, int destination) {
		auto* mutex = mutexMap.at(id);
		if(mutex->locked()) {
			mutex->acquireRequests().push(request_t(id, destination));
		} else {
			respondToAcquire(id, destination);
		}
	}


	/*
	 * Sends an acquire response to the destination proc, reading data using the
	 * resourceManager.
	 */
	template<typename T>
	void MutexServer<T>::respondToAcquire(DistributedId id, int destination) {
		auto* mutex = mutexMap.at(id);
		this->mutex_server_base::lock(mutex);
		const T& data = mutex->data();
		mpi.send(data, destination, epoch | Tag::ACQUIRE_RESPONSE);
	}
}
#endif
