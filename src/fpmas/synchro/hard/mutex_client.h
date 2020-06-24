#ifndef MUTEX_CLIENT_H
#define MUTEX_CLIENT_H

#include "fpmas/utils/macros.h"
#include "fpmas/api/communication/communication.h"
#include "fpmas/api/synchro/hard/enums.h"
#include "fpmas/api/synchro/hard/hard_sync_mutex.h"
#include "data_update_pack.h"
#include "fpmas/utils/log.h"

namespace fpmas::synchro::hard {
	using fpmas::api::synchro::hard::Epoch;
	using fpmas::api::synchro::hard::Tag;
	
	template<typename T>
		class MutexClient :
			public api::synchro::hard::MutexClient<T> {
				typedef api::synchro::hard::MutexServer<T>
					MutexServerBase;
				typedef api::synchro::hard::MutexClient<T>
					MutexClientBase;
				typedef api::synchro::hard::HardSyncMutex<T>
					HardSyncMutex;
				typedef api::synchro::hard::MutexRequest
					MutexRequest;
				typedef api::communication::MpiCommunicator MpiCommunicator;
				typedef api::communication::TypedMpi<DistributedId> IdMpi;
				typedef api::communication::TypedMpi<T> DataMpi;
				typedef api::communication::TypedMpi<DataUpdatePack<T>> DataUpdateMpi;

				private:
				MpiCommunicator& comm;
				IdMpi& id_mpi;
				DataMpi& data_mpi;
				DataUpdateMpi& data_update_mpi;
				MutexServerBase& mutex_server;

				void waitSendRequest(MPI_Request*);

				public:
				MutexClient(
						MpiCommunicator& comm, IdMpi& id_mpi, DataMpi& data_mpi, DataUpdateMpi& data_update_mpi,
						MutexServerBase& mutex_server)
					: comm(comm), id_mpi(id_mpi), data_mpi(data_mpi), data_update_mpi(data_update_mpi),
					mutex_server(mutex_server) {}

				T read(DistributedId, int location) override;
				void releaseRead(DistributedId, int location) override;

				T acquire(DistributedId, int location) override;
				void releaseAcquire(DistributedId, const T& data, int location) override;

				void lock(DistributedId, int location) override;
				void unlock(DistributedId, int location) override;

				void lockShared(DistributedId, int location) override;
				void unlockShared(DistributedId, int location) override;
			};

	template<typename T>
		T MutexClient<T>::read(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "reading data %s from %i", ID_C_STR(id), location);
			// Starts non-blocking synchronous send
			MPI_Request req;
			this->id_mpi.Issend(id, location, mutex_server.getEpoch() | Tag::READ, &req);

			// Keep responding to other READ / ACQUIRE request to avoid deadlock,
			// until the request has been received
			this->waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			// TODO : this is not thread safe.
			MPI_Status read_response_status;
			this->comm.probe(location, mutex_server.getEpoch() | Tag::READ_RESPONSE, &read_response_status);

			return data_mpi.recv(&read_response_status);
		}

	template<typename T>
		void MutexClient<T>::releaseRead(DistributedId id, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "releasing read data %s from %i", ID_C_STR(id), location);

			MPI_Request req;
			id_mpi.Issend(id, location, mutex_server.getEpoch() | Tag::UNLOCK_SHARED, &req);

			this->waitSendRequest(&req);
		}

	template<typename T>
		T MutexClient<T>::acquire(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "acquiring data %s from %i", ID_C_STR(id), location);
			// Starts non-blocking synchronous send
			MPI_Request req;
			this->id_mpi.Issend(id, location, mutex_server.getEpoch() | Tag::ACQUIRE, &req);

			this->waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			// TODO : this is not thread safe.
			MPI_Status acquire_response_status;
			this->comm.probe(location, mutex_server.getEpoch() | Tag::ACQUIRE_RESPONSE, &acquire_response_status);

			return data_mpi.recv(&acquire_response_status);
		}

	/**
	 * Gives back a previously acquired data to location proc, sending potential
	 * updates that occurred while data was locally acquired.
	 *
	 * @param id id of the data to read
	 * @param location rank of the data location
	 */
	template<typename T>
	void MutexClient<T>::releaseAcquire(DistributedId id, const T& data, int location) {
		FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "releasing acquired data %s from %i", ID_C_STR(id), location);
		DataUpdatePack<T> update {id, data};

		MPI_Request req;
		data_update_mpi.Issend(update, location, mutex_server.getEpoch() | Tag::RELEASE_ACQUIRE, &req);

		this->waitSendRequest(&req);
	}

	template<typename T>
	void MutexClient<T>::lock(DistributedId id, int location) {
		FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "locking data %s from %i", ID_C_STR(id), location);
		MPI_Request req;
		this->id_mpi.Issend(id, location, mutex_server.getEpoch() | Tag::LOCK, &req);

		this->waitSendRequest(&req);

		// The request has been received : it is assumed that the receiving proc is
		// now responding so we can safely wait for response without deadlocking
		// TODO : this is not thread safe.
		MPI_Status lock_response_status;
		this->comm.probe(location, mutex_server.getEpoch() | Tag::LOCK_RESPONSE, &lock_response_status);

		comm.recv(&lock_response_status);
	}

	template<typename T>
	void MutexClient<T>::unlock(DistributedId id, int location) {
		FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "unlocking data %s from %i", ID_C_STR(id), location);
		MPI_Request req;
		this->id_mpi.Issend(id, location, mutex_server.getEpoch() | Tag::UNLOCK, &req);

		this->waitSendRequest(&req);
	}

	template<typename T>
		void MutexClient<T>::lockShared(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "share locking data %s from %i", ID_C_STR(id), location);
			MPI_Request req;
			this->id_mpi.Issend(id, location, mutex_server.getEpoch() | Tag::LOCK_SHARED, &req);

			this->waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			MPI_Status lock_response_status;
			this->comm.probe(location, mutex_server.getEpoch() | Tag::LOCK_SHARED_RESPONSE, &lock_response_status);

			comm.recv(&lock_response_status);
		}

	template<typename T>
		void MutexClient<T>::unlockShared(DistributedId id, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "share unlocking data %s from %i", ID_C_STR(id), location);
			MPI_Request req;
			this->id_mpi.Issend(id, location, mutex_server.getEpoch() | Tag::UNLOCK_SHARED, &req);

			this->waitSendRequest(&req);
		}
	/*
	 * Allows to respond to other request while a request (sent in a synchronous
	 * message) is sending, in order to avoid deadlock.
	 */
	template<typename T>
	void MutexClient<T>::waitSendRequest(MPI_Request* req) {
		FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "wait for send...");
		bool sent = comm.test(req);

		while(!sent) {
			mutex_server.handleIncomingRequests();
			sent = comm.test(req);
		}
		FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "Request sent.");
	}

}
#endif
