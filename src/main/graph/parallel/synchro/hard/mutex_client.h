#ifndef MUTEX_CLIENT_H
#define MUTEX_CLIENT_H

#include "utils/macros.h"
#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/hard/enums.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"
#include "data_update_pack.h"

namespace FPMAS::graph::parallel::synchro::hard {
	using FPMAS::api::graph::parallel::synchro::hard::Epoch;
	using FPMAS::api::graph::parallel::synchro::hard::Tag;
	
	template<typename T, template<typename> class Mpi>
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
				typedef FPMAS::api::communication::MpiCommunicator comm_t;

				private:
				Epoch epoch = Epoch::EVEN;
				std::unordered_map<DistributedId, hard_sync_mutex*> mutexMap;
				comm_t& comm;
				Mpi<T> dataMpi {comm};
				Mpi<DataUpdatePack<T>> dataUpdateMpi {comm};
				mutex_server_base& mutexServer;

				void waitSendRequest(MPI_Request*);

				public:
				MutexClient(comm_t& comm, mutex_server_base& mutexServer)
					: comm(comm), mutexServer(mutexServer) {}

				const Mpi<T>& getDataMpi() const {return dataMpi;}
				const Mpi<DataUpdatePack<T>>& getDataUpdateMpi() const {return dataUpdateMpi;}

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

	template<typename T, template<typename> class Mpi>
		T MutexClient<T, Mpi>::read(DistributedId id, int location) {
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

			return dataMpi.recv(&read_response_status);
		}

	template<typename T, template<typename> class Mpi>
		T MutexClient<T, Mpi>::acquire(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "ACQUIRE", "acquiring data %s from %i", ID_C_STR(id), location);
			// Starts non-blocking synchronous send
			MPI_Request req;
			this->comm.Issend(id, location, epoch | Tag::ACQUIRE, &req);

			this->waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			MPI_Status acquire_response_status;
			this->comm.probe(location, epoch | Tag::ACQUIRE_RESPONSE, &acquire_response_status);

			return dataMpi.recv(&acquire_response_status);
		}

	/**
	 * Gives back a previously acquired data to location proc, sending potential
	 * updates that occurred while data was locally acquired.
	 *
	 * @param id id of the data to read
	 * @param location rank of the data location
	 */
	template<typename T, template<typename> class Mpi>
	void MutexClient<T, Mpi>::release(DistributedId id, int location) {
		std::cout << "release " << id << std::endl;
		DataUpdatePack<T> update {id, mutexMap.at(id)->data()};
		std::cout << "ok" << std::endl;

		MPI_Request req;
		dataUpdateMpi.Issend(update, location, epoch | Tag::RELEASE, &req);

		this->waitSendRequest(&req);
	}

	template<typename T, template<typename> class Mpi>
	void MutexClient<T, Mpi>::lock(DistributedId id, int location) {
		MPI_Request req;
		this->comm.Issend(id, location, epoch | Tag::LOCK, &req);

		this->waitSendRequest(&req);

		// The request has been received : it is assumed that the receiving proc is
		// now responding so we can safely wait for response without deadlocking
		MPI_Status lock_response_status;
		this->comm.probe(location, epoch | Tag::LOCK_RESPONSE, &lock_response_status);

		comm.recv(&lock_response_status);
	}

	template<typename T, template<typename> class Mpi>
	void MutexClient<T, Mpi>::unlock(DistributedId id, int location) {
		MPI_Request req;
		this->comm.Issend(id, location, epoch | Tag::UNLOCK, &req);

		this->waitSendRequest(&req);
	}

	/*
	 * Allows to respond to other request while a request (sent in a synchronous
	 * message) is sending, in order to avoid deadlock.
	 */
	template<typename T, template<typename> class Mpi>
	void MutexClient<T, Mpi>::waitSendRequest(MPI_Request* req) {
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
