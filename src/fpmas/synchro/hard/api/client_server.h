#ifndef FPMAS_CLIENT_SERVER_API_H
#define FPMAS_CLIENT_SERVER_API_H

/** \file src/fpmas/synchro/hard/api/client_server.h
 * ClientServer API
 */

#include "enums.h"
#include "fpmas/api/graph/distributed_edge.h"

namespace fpmas { namespace synchro { namespace hard { namespace api {
	/**
	 * Structure used to represent any type of request to an HardSyncMutex.
	 */
	struct MutexRequest {
		/**
		 * Special source value used to indicate that the request is coming
		 * from the local process.
		 */
		static const int LOCAL;
		/**
		 * Id of the requested node.
		 */
		DistributedId id;
		/**
		 * Rank of the process from which the request is coming.
		 */
		int source;
		/**
		 * Request type.
		 */
		MutexRequestType type;

		/**
		 * MutexRequest constructor.
		 *
		 * @param id id of the requested node
		 * @param source rank of the process from which the request is coming
		 * @param type request type
		 */
		MutexRequest(DistributedId id, int source, MutexRequestType type)
			: id(id), source(source), type(type) {}

		/**
		 * Checks equality between two MutexRequests.
		 *
		 * Two MutexRequests are equals if and only if they have the same
		 * `type`, `id` and `source`.
		 *
		 * @param other MutexRequest to compare to this
		 * @return true iff other is equal to this
		 */
		bool operator==(const MutexRequest& other) const {
			return
				(this->type == other.type) &&
				(this->id == other.id) &&
				(this->source == other.source);
		}
	};

	/**
	 * MutexClient API.
	 *
	 * The MutexClient is used to transmit requests from an HardSyncMutex
	 * instance (associated to a \DISTANT node) to distant processes.
	 *
	 * All request transmission functions blocks until the corresponding
	 * response is received from the corresponding `location`.
	 *
	 * \image html mutex_client_server_arch.png "General request transmission architecture"
	 */
	template<typename T>
		class MutexClient {
			public:
				/**
				 * Transmits a READ request of node `id` to its `location`.
				 *
				 * @param id of the node to read
				 * @param location rank of the process that own node `id`
				 * @return read data
				 */
				virtual T read(DistributedId id, int location) = 0;

				/**
				 * Transmits a RELEASE_READ request of node `id` to its `location`.
				 *
				 * @param id of the read node
				 * @param location rank of the process that own node `id`
				 */
				virtual void releaseRead(DistributedId id, int location) = 0;

				/**
				 * Transmits an ACQUIRE request of node `id` to its `location`.
				 *
				 * @param id of the node to acquire
				 * @param location rank of the process that own node `id`
				 * @return acquired data
				 */
				virtual T acquire(DistributedId id, int location) = 0;

				/**
				 * Transmits a RELEASE_ACQUIRE request of node `id` to its `location`.
				 *
				 * The provided `updated_data` is serialized and transmitted to
				 * commit write operation to the owner process.
				 *
				 * @param id of the acquired node
				 * @param updated_data reference to the locally updated data
				 * @param location rank of the process that own node `id`
				 */
				virtual void releaseAcquire(DistributedId id, const T& updated_data, int location) = 0;

				/**
				 * Transmits a LOCK request of node `id` to its `location`.
				 *
				 * @param id of the node to lock
				 * @param location rank of the process that own node `id`
				 */
				virtual void lock(DistributedId id, int location) = 0;

				/**
				 * Transmits an UNLOCK request of node `id` to its `location`.
				 *
				 * @param id of the locked node
				 * @param location rank of the process that own node `id`
				 */
				virtual void unlock(DistributedId id, int location) = 0;

				/**
				 * Transmits a LOCK_SHARED request of node `id` to its `location`.
				 *
				 * @param id of the node to share lock
				 * @param location rank of the process that own node `id`
				 */
				virtual void lockShared(DistributedId id, int location) = 0;

				/**
				 * Transmits an UNLOCK_SHARED request of node `id` to its `location`.
				 *
				 * @param id of the share locked node
				 * @param location rank of the process that own node `id`
				 */
				virtual void unlockShared(DistributedId id, int location) = 0;

				virtual ~MutexClient() {}
		};

	/**
	 * Generic server API.
	 *
	 * This interface is notably used by the generic TerminationAlgorithm to keep
	 * handling requests of any server until termination is detected.
	 *
	 * Since termination is determined by "step", an odd or even Epoch is used
	 * to describe the current state of the server, so that messages sent in an
	 * odd period can't be received in an even period and vice versa.
	 *
	 * the handleIncomingRequests() method is also extensively used to prevent
	 * deadlock situations.
	 */
	class Server {
		public:
			/**
			 * Sets the current Epoch of the server.
			 *
			 * @param epoch current Epoch
			 */
			virtual void setEpoch(Epoch epoch) = 0;
			/**
			 * Gets the current Epoch of the server.
			 *
			 * @return current epoch
			 */
			virtual Epoch getEpoch() const = 0;

			/**
			 * Performs the full reception cycle associated to this server.
			 *
			 * If at least one request supposed to be handled by this server is
			 * pending, the server **must** handle at least one of those
			 * request to ensure progress and avoid deadlock situations.
			 */
			virtual void handleIncomingRequests() = 0;
	};

	template<typename T> class HardSyncMutex;

	/**
	 * MutexServer API.
	 *
	 * The purpose of the MutexServer is to respond to requests incoming from
	 * other processes.
	 *
	 * More precisely, the MutexServer responds to requests if the requested
	 * HardSyncMutex is available, or else enqueue the request. Moreover, it is
	 * the role of the MutexServer to unqueue pending requests when the
	 * HardSyncMutex becomes available again.
	 *
	 * Currently, the MutexServer is assumed to run directly in the main
	 * thread, assuming a single-threaded environment. In consequence, request
	 * are handled only when the MutexClient is performing some requests, to
	 * avoid deadlock, and during the termination process of the
	 * TerminationAlgorithm.
	 */
	template<typename T>
		class MutexServer : public virtual Server {
			friend HardSyncMutex<T>;
			protected:
			/**
			 * Internally locks the mutex.
			 *
			 * @param mutex to lock
			 */
			void lock(HardSyncMutex<T>* mutex) {mutex->_lock();}
			/**
			 * Internally unlocks the mutex.
			 *
			 * @param mutex to unlock
			 */
			void lockShared(HardSyncMutex<T>* mutex) {mutex->_lockShared();}
			/**
			 * Internally share locks the mutex.
			 *
			 * @param mutex to share lock
			 */
			void unlock(HardSyncMutex<T>* mutex) {mutex->_unlock();}
			/**
			 * Internally share unlocks the mutex.
			 *
			 * @param mutex to share unlock
			 */
			void unlockShared(HardSyncMutex<T>* mutex) {mutex->_unlockShared();}
			public:
			/**
			 * Adds the provided `mutex`, associated to the resource `id`, to
			 * the current set of managed mutexes.
			 *
			 * A mutex is "managed" means that the MutexServer instance is able
			 * to receive and handle requests involving this mutex.
			 *
			 * @param id id of the node associated to mutex
			 * @param mutex mutex to manage
			 */
			virtual void manage(DistributedId id, HardSyncMutex<T>* mutex) = 0;
			/**
			 * Removes the mutex associated to the specified id from the set of
			 * managed mutexes.
			 *
			 * @param id id of the node associated to the mutex to remove
			 */
			virtual void remove(DistributedId id) = 0;

			/**
			 * Waits for the specified request to be handled.
			 *
			 * This is notably used in a single-threaded environment from the
			 * main thread, where the argument `request` is a request performed
			 * by the local process. Indeed, the local process cannot directly
			 * access its own resources, since they might be acquired by other
			 * processes.  To solve this issue, the HardSyncMutex adds a
			 * "LOCAL" request to its waiting queue, and "waits" for this
			 * request to be handled by the MutexServer to get back to its main
			 * execution thread. While "waiting", the MutexServer keeps
			 * handling and unqueueing pending requests until the specified
			 * `request` is reached.
			 *
			 * @param request request to wait for
			 */
			virtual void wait(const MutexRequest& request) = 0;

			/**
			 * Notifies the MutexServer that the local process has released the
			 * local mutex associated to the provided id.
			 *
			 * This is used to trigger pending requests handling.
			 *
			 * @param id id of the node associated to the released mutex
			 */
			virtual void notify(DistributedId id) = 0;


			virtual ~MutexServer() {}
		};

	/**
	 * LinkClient API.
	 *
	 * The LinkClient is used to transmit link, unlink and node removal
	 * requests from the local process to distant processes.
	 */
	template<typename T>
		class LinkClient {
			public:
				/**
				 * Transmits a link request for the specified `edge`.
				 *
				 * The provided edge is assumed to be \DISTANT, behavior is
				 * undefined otherwise.
				 *
				 * The request is then transmitted to two other processes if
				 * source and target nodes are \DISTANT and located on different
				 * processes, or to one process if they are located on the same
				 * process.
				 *
				 * In any case, it is guaranteed that the edge is properly
				 * linked on any process involved upon return.
				 *
				 * Notice that the api::graph::DistributedGraph implementation
				 * ensures that source and target nodes are share locked when
				 * this method is called.
				 *
				 * @param edge edge to link
				 */
				virtual void link(const fpmas::api::graph::DistributedEdge<T>* edge) = 0;

				/**
				 * Transmits an unlink request for the specified `edge`.
				 *
				 * The provided edge is assumed to be \DISTANT, behavior is
				 * undefined otherwise.
				 *
				 * The request is then transmitted to two other processes if
				 * source and target nodes are \DISTANT and located on different
				 * processes, or to one process if they are located on the same
				 * process.
				 *
				 * In any case, it is guaranteed that the edge is properly
				 * unlinked on any process involved upon return.
				 *
				 * Notice that the api::graph::DistributedGraph implementation
				 * ensures that source and target nodes are share locked when
				 * this method is called.
				 *
				 * @param edge edge to unlink
				 */
				virtual void unlink(const fpmas::api::graph::DistributedEdge<T>* edge) = 0;

				virtual ~LinkClient() {};
		};

	/**
	 * LinkServer API.
	 *
	 * Adds nothing to the Server API.
	 */
	typedef Server LinkServer;

	/**
	 * Generic termination algorithm.
	 */
	class TerminationAlgorithm {
		public:
			/**
			 * Apply the termination algorithm to the provided server.
			 *
			 * Termination is determined when all the processes have entered
			 * this function and no more request is pending in the global
			 * process. In consequence, the argument server must still be able
			 * to respond to requests from processes that have not terminated
			 * yet while the termination algorithm is applied to ensure
			 * progress. This can be performed in practice by the termination
			 * algorithm using the Server::handleIncomingRequests() method.
			 *
			 * Moreover, the termination algorithm must toggle the Server Epoch
			 * once termination has been detected.
			 *
			 * @param server server to terminate
			 */
			virtual void terminate(Server& server) = 0;

			virtual ~TerminationAlgorithm() {};
	};
}}}}
#endif
