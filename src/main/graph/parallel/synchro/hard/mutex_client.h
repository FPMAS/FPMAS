#ifndef MUTEX_CLIENT_H
#define MUTEX_CLIENT_H

#include "utils/macros.h"
#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/hard/enums.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {
	using FPMAS::api::graph::parallel::synchro::hard::Epoch;
	using FPMAS::api::graph::parallel::synchro::hard::Tag;

	template<typename T>
		class MutexClient :
			public FPMAS::api::graph::parallel::synchro::hard::MutexClient<T> {
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T>
					mutex_server_base;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexClient<T>
					mutex_client_base;
				typedef FPMAS::api::graph::parallel::synchro::hard::HardSyncMutex<T>
					hard_sync_mutex;
				typedef FPMAS::api::graph::parallel::synchro::hard::Request
					request_t;
				typedef FPMAS::api::communication::Mpi<T> mpi_t;
				typedef FPMAS::api::communication::MpiCommunicator comm_t;

				private:
				Epoch epoch = Epoch::EVEN;
				std::unordered_map<DistributedId, hard_sync_mutex*> mutexMap;
				mpi_t& mpi;
				comm_t& comm;
				mutex_server_base& mutexServer;

				void waitSendRequest(MPI_Request*);

				public:
				MutexClient(mpi_t& mpi, comm_t& comm, mutex_server_base& mutexServer)
					: mpi(mpi), comm(comm), mutexServer(mutexServer) {}

				void manage(DistributedId id, hard_sync_mutex* mutex) override {
					mutexMap.insert({id, mutex});
				}
				void remove(DistributedId id) override {
					mutexMap.erase(id);
				}

				T read(DistributedId, int location) override;
				T acquire(DistributedId, int location) override;
				void release(DistributedId, int location) override;

				void lock(DistributedId, int location) override;
				void unlock(DistributedId, int location) override;
			};

	template<typename T>
		T MutexClient<T>::read(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "READ", "reading data %s from %i", ID_C_STR(id), location);
			// Starts non-blocking synchronous send
			MPI_Request req;
			this->comm.Issend(id, location, epoch | Tag::READ, &req);

			// Keep responding to other READ / ACQUIRE request to avoid deadlock,
			// until the request has been received
			this->waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			MPI_Status read_response_status;
			this->comm.probe(location, epoch | Tag::READ_RESPONSE, &read_response_status);

			return mpi.recv(&read_response_status);
		}

	template<typename T>
		T MutexClient<T>::acquire(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "acquiring data %s from %i", ID_C_STR(id), location);
			// Starts non-blocking synchronous send
			MPI_Request req;
			this->comm.Issend(id, location, epoch | Tag::ACQUIRE, &req);

			this->waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			MPI_Status acquire_response_status;
			this->comm.probe(location, epoch | Tag::ACQUIRE_RESPONSE, &acquire_response_status);

			return mpi.recv(&acquire_response_status);
		}

	/**
	 * Gives back a previously acquired data to location proc, sending potential
	 * updates that occurred while data was locally acquired.
	 *
	 * @param id id of the data to read
	 * @param location rank of the data location
	 */
	template<typename T>
	void MutexClient<T>::release(DistributedId id, int location) {
		// Perform the response
   /*     nlohmann::json update = {*/
			//{"id", id},
			//{"data", this->resourceHandler.getUpdatedData(id)}
	  /*  }*/;
		//std::string data = update.dump();
		//FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "giving back to %i : %s", location, data.c_str());
		//MPI_Request req;
		// TODO : asynchronous send : is this a pb? => maybe.
		//this->comm.Issend(data, location, epoch | Tag::ACQUIRE_GIVE_BACK, &req);
		const T& updatedData = mutexMap.at(id)->data();

		MPI_Request req;
		this->mpi.Issend(updatedData, location, epoch | Tag::RELEASE, &req);

		this->waitSendRequest(&req);
	}

	template<typename T>
	void MutexClient<T>::lock(DistributedId id, int location) {
		MPI_Request req;
		this->comm.Issend(id, location, epoch | Tag::LOCK, &req);

		this->waitSendRequest(&req);

		// The request has been received : it is assumed that the receiving proc is
		// now responding so we can safely wait for response without deadlocking
		MPI_Status lock_response_status;
		this->comm.probe(location, epoch | Tag::LOCK_RESPONSE, &lock_response_status);

		comm.recv(&lock_response_status);
	}

	template<typename T>
	void MutexClient<T>::unlock(DistributedId id, int location) {
		MPI_Request req;
		this->comm.Issend(id, location, epoch | Tag::UNLOCK, &req);

		this->waitSendRequest(&req);
	}

	/*
	 * Allows to respond to other request while a request (sent in a synchronous
	 * message) is sending, in order to avoid deadlock.
	 */
	template<typename T>
	void MutexClient<T>::waitSendRequest(MPI_Request* req) {
		FPMAS_LOGD(this->comm.getRank(), "WAIT", "wait for send");
		MPI_Status send_status;
		bool sent = comm.test(req);

		while(!sent) {
			mutexServer.handleIncomingRequests();
			sent = comm.test(req);
		}
	}

}
#endif
