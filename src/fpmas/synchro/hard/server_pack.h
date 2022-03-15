#ifndef FPMAS_SERVER_PACK_H
#define FPMAS_SERVER_PACK_H

/** \file src/fpmas/synchro/hard/server_pack.h
 * ServerPack implementation.
 */

#include "fpmas/api/communication/communication.h"
#include "./api/client_server.h"
#include "fpmas/utils/log.h"

namespace fpmas { namespace synchro { namespace hard {

	/**
	 * An api::Server implementation that does not handle any request.
	 */
	class VoidServer : public api::Server {
		public:
			void setEpoch(api::Epoch) override {};
			api::Epoch getEpoch() const override {
				return {};
			};
			void handleIncomingRequests() override {};
	};

	/**
	 * A Server implementation wrapping an api::MutexServer and an
	 * api::LinkServer.
	 *
	 * This is used to apply an api::TerminationAlgorithm to the two instances
	 * simultaneously. More precisely, when applying the termination algorithm
	 * to this server, the system is still able to answer to mutex AND link
	 * requests, to completely determine global termination.
	 *
	 * The wait*() methods also allows to easily wait for point-to-point
	 * communications initiated by either server without deadlock, by ensuring
	 * progression on both servers.
	 */
	class ServerPackBase : public api::Server {
		typedef api::Epoch Epoch;

		private:
		std::vector<fpmas::api::communication::Request> pending_requests;

		fpmas::api::communication::MpiCommunicator& comm;
		api::TerminationAlgorithm& termination;
		api::Server& mutex_server;
		api::Server& link_server;
		Epoch epoch;

		public:
		/**
		 * ServerPackBase constructor.
		 *
		 * @param comm MPI communicator
		 * @param termination termination algorithm used to terminated
		 * this server, terminating both mutex_server and link_server
		 * at once.
		 * @param mutex_server Mutex server
		 * @param link_server Link server
		 */
		ServerPackBase(
				fpmas::api::communication::MpiCommunicator& comm,
				api::TerminationAlgorithm& termination,
				api::Server& mutex_server,
				api::Server& link_server)
			: comm(comm), termination(termination), mutex_server(mutex_server), link_server(link_server) {
				setEpoch(Epoch::EVEN);
			}

		/**
		 * Reference to the internal api::MutexServer instance.
		 */
		virtual api::Server& mutexServer() {
			return mutex_server;
		}

		/**
		 * Reference to the internal api::LinkServer instance.
		 */
		virtual api::Server& linkServer() {
			return link_server;
		}

		/**
		 * Performs a reception cycle on the api::MutexServer AND on the
		 * api::LinkServer.
		 */
		void handleIncomingRequests() override {
			mutex_server.handleIncomingRequests();
			link_server.handleIncomingRequests();
		}

		/**
		 * Sets the Epoch of the api::MutexServer AND the
		 * api::LinkServer.
		 *
		 * @param epoch new epoch
		 */
		void setEpoch(Epoch epoch) override {
			this->epoch = epoch;
			mutex_server.setEpoch(epoch);
			link_server.setEpoch(epoch);
		}

		/**
		 * Gets the common epoch of the api::MutexServer and the
		 * api::LinkServer.
		 *
		 * @return current Epoch
		 */
		Epoch getEpoch() const override {
			return epoch;
		}

		/**
		 * Applies the termination algorithm to this ServerPack.
		 */
		void terminate() {
			FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "wait all...", "");
			// Applies the termination algorithm: keeps handling requests
			termination.terminate(*this);

			// Completes Isend responses performed while handling requests
			// since the last terminate() call.
			comm.waitAll(pending_requests);
			pending_requests.clear();
		}

		/**
		 * Returns a list containing all the pending non-blocking requests,
		 * that are waiting for completions.
		 *
		 * Such non-blocking communications are notably used to send responses
		 * to READ and ACQUIRE requests.
		 *
		 * Pending requests are guaranteed to be completed at the latest upon
		 * return of the next terminate() call, so that Request buffers can be
		 * freed.
		 *
		 * It is also valid to complete requests before the next terminate()
		 * call, for example using
		 * fpmas::api::communication::MpiCommunicator::test() or
		 * fpmas::api::communication::MpiCommunicator::testSome() in order to
		 * limit memory usage, as long as this cannot produce deadlock
		 * situations.
		 */
		std::vector<fpmas::api::communication::Request>& pendingRequests() {
			return pending_requests;
		}

		/**
		 * Method used to wait for an MPI_Request to be sent.
		 *
		 * The request might be performed by the api::MutexServer OR the
		 * api::LinkServer. In any case, this methods handles incoming
		 * requests (see api::MutexServer::handleIncomingRequests() and
		 * api::LinkServer::handleIncomingRequests()) until the MPI_Request
		 * is complete, in order to avoid deadlock.
		 *
		 * @param req MPI request to complete
		 */
		void waitSendRequest(fpmas::api::communication::Request& req) {
			FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "wait for send...", "");
			bool sent = comm.test(req);

			while(!sent) {
				handleIncomingRequests();
				sent = comm.test(req);
			}
			FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "Request sent.", "");
		}

		/**
		 * Method used to wait for request responses with data (e.g. to
		 * READ, ACQUIRE) from the specified `mpi` instance to be
		 * available.
		 *
		 * The request might be performed by the api::MutexServer OR the
		 * api::LinkServer. In any case, this methods handles incoming
		 * requests (see api::MutexServer::handleIncomingRequests() and
		 * api::LinkServer::handleIncomingRequests()) until a response
		 * is available, in order to avoid deadlock.
		 *
		 * Upon return, it is safe to receive the response with
		 * api::communication::MpiCommunicator::recv() using the same source and tag.
		 *
		 * @param mpi TypedMpi instance to probe
		 * @param source rank of the process from which the response should
		 * be received
		 * @param tag response tag (READ_RESPONSE, ACQUIRE_RESPONSE, etc)
		 * @param status pointer to MPI_Status, passed to
		 * api::communication::MpiCommunicator::Iprobe
		 */
		template<typename T>
			void waitResponse(
					fpmas::api::communication::TypedMpi<T>& mpi,
					int source, api::Tag tag,
					fpmas::api::communication::Status& status) {
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "wait for response...", "");
				bool response_available = mpi.Iprobe(source, getEpoch() | tag, status);

				while(!response_available) {
					handleIncomingRequests();
					response_available = mpi.Iprobe(source, getEpoch() | tag, status);
				}
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "Response available.", "");
			}

		/**
		 * Method used to wait for request responses without data (e.g. to
		 * LOCK, UNLOCK) from the specified `comm` instance to be
		 * available.
		 *
		 * The request might be performed by the api::MutexServer OR the
		 * api::LinkServer. In any case, this methods handles incoming
		 * requests (see api::MutexServer::handleIncomingRequests() and
		 * api::LinkServer::handleIncomingRequests()) until a response
		 * is available, in order to avoid deadlock.
		 *
		 * Upon return, it is safe to receive the response with
		 * api::communication::MpiCommunicator::recv() using the same source and tag.
		 *
		 * @param comm MpiCommunicator instance to probe
		 * @param source rank of the process from which the response should
		 * be received
		 * @param tag response tag (READ_RESPONSE, ACQUIRE_RESPONSE, etc)
		 * @param status pointer to MPI_Status, passed to
		 * api::communication::MpiCommunicator::Iprobe
		 */
		void waitVoidResponse(
				fpmas::api::communication::MpiCommunicator& comm,
				int source, api::Tag tag,
				fpmas::api::communication::Status& status) {
			FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "wait for response...", "");
			bool response_available = comm.Iprobe(
					fpmas::api::communication::MpiCommunicator::IGNORE_TYPE,
					source, getEpoch() | tag, status);

			while(!response_available) {
				handleIncomingRequests();
				response_available = comm.Iprobe(
						fpmas::api::communication::MpiCommunicator::IGNORE_TYPE,
						source, getEpoch() | tag, status);
			}
			FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "Response available.", "");
		}

	};

	
	/**
	 * A ServerPackBase extension that enforces MutexServer and LinkServer
	 * types.
	 */
	template<typename MutexServer, typename LinkServer>
		class ServerPack : public ServerPackBase {
			private:
				MutexServer& mutex_server;
				LinkServer& link_server;

			public:
				/**
				 * ServerPack constructor.
				 *
				 * @param comm MPI communicator
				 * @param termination termination algorithm used to terminated
				 * this server, terminating both mutex_server and link_server
				 * at once.
				 * @param mutex_server Mutex server
				 * @param link_server Link server
				 */
				ServerPack(
						fpmas::api::communication::MpiCommunicator& comm,
						api::TerminationAlgorithm& termination,
						MutexServer& mutex_server, LinkServer& link_server) :
					ServerPackBase(comm, termination, mutex_server, link_server),
					mutex_server(mutex_server), link_server(link_server) {
					}

				MutexServer& mutexServer() override {
					return mutex_server;
				}

				LinkServer& linkServer() override {
					return link_server;
				}
		};
}}}
#endif
