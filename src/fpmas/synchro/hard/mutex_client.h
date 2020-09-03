#ifndef FPMAS_MUTEX_CLIENT_H
#define FPMAS_MUTEX_CLIENT_H

/** \file src/fpmas/synchro/hard/mutex_client.h
 * MutexClient implementation.
 */

#include "fpmas/utils/macros.h"
#include "fpmas/synchro/hard/api/hard_sync_mode.h"
#include "fpmas/utils/data_update_pack.h"
#include "server_pack.h"

namespace fpmas { namespace synchro { namespace hard {
	using api::Epoch;
	using api::Tag;

	/**
	 * api::MutexClient implementation.
	 */
	template<typename T>
		class MutexClient :
			public api::MutexClient<T> {
				typedef api::MutexServer<T> MutexServerBase;
				typedef api::MutexClient<T> MutexClientBase;
				typedef api::HardSyncMutex<T> HardSyncMutex;
				typedef api::MutexRequest MutexRequest;
				typedef fpmas::api::communication::MpiCommunicator MpiCommunicator;
				typedef fpmas::api::communication::TypedMpi<DistributedId> IdMpi;
				typedef fpmas::api::communication::TypedMpi<T> DataMpi;
				typedef fpmas::api::communication::TypedMpi<DataUpdatePack<T>> DataUpdateMpi;

				private:
				MpiCommunicator& comm;
				IdMpi& id_mpi;
				DataMpi& data_mpi;
				DataUpdateMpi& data_update_mpi;
				ServerPack<T>& server_pack;

				public:
				/**
				 * MutexClient constructor.
				 *
				 * @param comm MPI communicator
				 * @param id_mpi IdMpi instance
				 * @param data_mpi DataMpi instance
				 * @param data_update_mpi DataUpdateMpi instance
				 * @param server_pack associated server pack
				 */
				MutexClient(
						MpiCommunicator& comm, IdMpi& id_mpi, DataMpi& data_mpi, DataUpdateMpi& data_update_mpi,
						ServerPack<T>& server_pack)
					: comm(comm), id_mpi(id_mpi), data_mpi(data_mpi), data_update_mpi(data_update_mpi),
					server_pack(server_pack) {}

				T read(DistributedId, int location) override;
				void releaseRead(DistributedId, int location) override;

				T acquire(DistributedId, int location) override;
				void releaseAcquire(DistributedId, const T& updated_data, int location) override;

				void lock(DistributedId, int location) override;
				void unlock(DistributedId, int location) override;

				void lockShared(DistributedId, int location) override;
				void unlockShared(DistributedId, int location) override;
			};

	template<typename T>
		T MutexClient<T>::read(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "reading node %s from %i", FPMAS_C_STR(id), location);
			// Starts non-blocking synchronous send
			fpmas::api::communication::Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::READ, req);

			// Keep responding to other READ / ACQUIRE request to avoid deadlock,
			// until the request has been received
			server_pack.waitSendRequest(req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			// TODO : this is not thread safe.
			fpmas::api::communication::Status read_response_status;
			server_pack.waitResponse(data_mpi, location, Tag::READ_RESPONSE, read_response_status);

			return data_mpi.recv(read_response_status.source, read_response_status.tag);
		}

	template<typename T>
		void MutexClient<T>::releaseRead(DistributedId id, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "releasing read node %s from %i", FPMAS_C_STR(id), location);

			fpmas::api::communication::Request req;
			id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::UNLOCK_SHARED, req);

			server_pack.waitSendRequest(req);
		}

	template<typename T>
		T MutexClient<T>::acquire(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "acquiring node %s from %i", FPMAS_C_STR(id), location);
			// Starts non-blocking synchronous send
			fpmas::api::communication::Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::ACQUIRE, req);

			server_pack.waitSendRequest(req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			// TODO : this is not thread safe.
			fpmas::api::communication::Status acquire_response_status;
			server_pack.waitResponse(data_mpi, location, Tag::ACQUIRE_RESPONSE, acquire_response_status);

			return data_mpi.recv(acquire_response_status.source, acquire_response_status.tag);
		}

	template<typename T>
		void MutexClient<T>::releaseAcquire(DistributedId id, const T& updated_data, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "releasing acquired node %s from %i", FPMAS_C_STR(id), location);
			DataUpdatePack<T> update {id, updated_data};

			fpmas::api::communication::Request req;
			data_update_mpi.Issend(update, location, server_pack.getEpoch() | Tag::RELEASE_ACQUIRE, req);

			server_pack.waitSendRequest(req);
		}

	template<typename T>
		void MutexClient<T>::lock(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "locking node %s from %i", FPMAS_C_STR(id), location);

			fpmas::api::communication::Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::LOCK, req);

			server_pack.waitSendRequest(req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			// TODO : this is not thread safe.
			fpmas::api::communication::Status lock_response_status;
			server_pack.waitVoidResponse(comm, location, Tag::LOCK_RESPONSE, lock_response_status);

			comm.recv(lock_response_status.source, lock_response_status.tag);
		}

	template<typename T>
		void MutexClient<T>::unlock(DistributedId id, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "unlocking node %s from %i", FPMAS_C_STR(id), location);

			fpmas::api::communication::Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::UNLOCK, req);

			server_pack.waitSendRequest(req);
		}

	template<typename T>
		void MutexClient<T>::lockShared(DistributedId id, int location) {
			FPMAS_LOGD(this->comm.getRank(), "MUTEX_CLIENT", "share locking node %s from %i", FPMAS_C_STR(id), location);

			fpmas::api::communication::Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::LOCK_SHARED, req);

			server_pack.waitSendRequest(req);

			// The request has been received : it is assumed that the receiving proc is
			// now responding so we can safely wait for response without deadlocking
			fpmas::api::communication::Status lock_response_status;
			server_pack.waitVoidResponse(comm, location, Tag::LOCK_SHARED_RESPONSE, lock_response_status);

			comm.recv(lock_response_status.source, lock_response_status.tag);
		}

	template<typename T>
		void MutexClient<T>::unlockShared(DistributedId id, int location) {
			FPMAS_LOGV(this->comm.getRank(), "MUTEX_CLIENT", "share unlocking node %s from %i", FPMAS_C_STR(id), location);

			fpmas::api::communication::Request req;
			this->id_mpi.Issend(id, location, server_pack.getEpoch() | Tag::UNLOCK_SHARED, req);

			server_pack.waitSendRequest(req);
		}
}}}
#endif
