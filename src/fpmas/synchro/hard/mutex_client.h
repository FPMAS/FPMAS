#ifndef MUTEX_CLIENT_H
#define MUTEX_CLIENT_H

#include "fpmas/utils/macros.h"
#include "fpmas/api/communication/communication.h"
#include "fpmas/api/synchro/hard/enums.h"
#include "fpmas/api/synchro/hard/hard_sync_mutex.h"
#include "data_update_pack.h"
#include "fpmas/utils/log.h"
#include "server_pack.h"

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
				ServerPack<T>& server_pack;

				//void waitSendRequest(MPI_Request*);

				public:
				MutexClient(
						MpiCommunicator& comm, IdMpi& id_mpi, DataMpi& data_mpi, DataUpdateMpi& data_update_mpi,
						ServerPack<T>& server_pack)
					: comm(comm), id_mpi(id_mpi), data_mpi(data_mpi), data_update_mpi(data_update_mpi),
					server_pack(server_pack) {}

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
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "reading node %s from %i", ID_C_STR(id), location);
			// Starts non-blocking synchronous send
			MPI_Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::READ, &req);

			// Keep responding to other READ / ACQUIRE request to avoid deadlock,
			// until the request has been received
			server_pack.waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			// TODO : this is not thread safe.
			MPI_Status read_response_status;
			this->comm.probe(location, server_pack.getEpoch() | Tag::READ_RESPONSE, &read_response_status);

			return data_mpi.recv(read_response_status.MPI_SOURCE, read_response_status.MPI_TAG);
		}

	template<typename T>
		void MutexClient<T>::releaseRead(DistributedId id, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "releasing read node %s from %i", ID_C_STR(id), location);

			MPI_Request req;
			id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::UNLOCK_SHARED, &req);

			server_pack.waitSendRequest(&req);
		}

	template<typename T>
		T MutexClient<T>::acquire(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "acquiring node %s from %i", ID_C_STR(id), location);
			// Starts non-blocking synchronous send
			MPI_Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::ACQUIRE, &req);

			server_pack.waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			// TODO : this is not thread safe.
			MPI_Status acquire_response_status;
			this->comm.probe(location, server_pack.getEpoch() | Tag::ACQUIRE_RESPONSE, &acquire_response_status);

			return data_mpi.recv(acquire_response_status.MPI_SOURCE, acquire_response_status.MPI_TAG);
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
		FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "releasing acquired node %s from %i", ID_C_STR(id), location);
		DataUpdatePack<T> update {id, data};

		MPI_Request req;
		data_update_mpi.Issend(update, location, server_pack.getEpoch() | Tag::RELEASE_ACQUIRE, &req);

		server_pack.waitSendRequest(&req);
	}

	template<typename T>
	void MutexClient<T>::lock(DistributedId id, int location) {
		FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "locking node %s from %i", ID_C_STR(id), location);
		MPI_Request req;
		this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::LOCK, &req);

		server_pack.waitSendRequest(&req);

		// The request has been received : it is assumed that the receiving proc is
		// now responding so we can safely wait for response without deadlocking
		// TODO : this is not thread safe.
		MPI_Status lock_response_status;
		this->comm.probe(location, server_pack.getEpoch() | Tag::LOCK_RESPONSE, &lock_response_status);

		comm.recv(lock_response_status.MPI_SOURCE, lock_response_status.MPI_TAG);
	}

	template<typename T>
	void MutexClient<T>::unlock(DistributedId id, int location) {
		FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "unlocking node %s from %i", ID_C_STR(id), location);
		MPI_Request req;
		this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::UNLOCK, &req);

		server_pack.waitSendRequest(&req);
	}

	template<typename T>
		void MutexClient<T>::lockShared(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "share locking node %s from %i", ID_C_STR(id), location);
			MPI_Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::LOCK_SHARED, &req);

			server_pack.waitSendRequest(&req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			MPI_Status lock_response_status;
			this->comm.probe(location, server_pack.getEpoch() | Tag::LOCK_SHARED_RESPONSE, &lock_response_status);

			comm.recv(lock_response_status.MPI_SOURCE, lock_response_status.MPI_TAG);
		}

	template<typename T>
		void MutexClient<T>::unlockShared(DistributedId id, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "share unlocking node %s from %i", ID_C_STR(id), location);
			MPI_Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::UNLOCK_SHARED, &req);

			server_pack.waitSendRequest(&req);
		}
}
#endif
