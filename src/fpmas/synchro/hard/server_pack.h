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
	 * A Server implementation wrapping an api::MutexServer and an
	 * api::LinkServer.
	 *
	 * This is used to apply an api::TerminationAlgorithm to the two instances
	 * simultaneously. More precisely, when applying the termination algorithm
	 * to this server, the system is still able to answer to mutex AND link
	 * requests, to completely determine global termination.
	 */
	template<typename T>
		class ServerPack : public api::Server {
			typedef api::Epoch Epoch;
			typedef api::TerminationAlgorithm TerminationAlgorithm;

			private:
			fpmas::api::communication::MpiCommunicator& comm;
			TerminationAlgorithm& termination;
			api::MutexServer<T>& mutex_server;
			api::LinkServer& link_server;
			Epoch epoch;

			public:
			/**
			 * ServerPack constructor.
			 *
			 * @param comm MPI communicator
			 * @param termination termination algorithm
			 * @param mutex_server Mutex server to terminate
			 * @param link_server Link server to terminate
			 */
			ServerPack(
					fpmas::api::communication::MpiCommunicator& comm,
					TerminationAlgorithm& termination,
					api::MutexServer<T>& mutex_server,
					api::LinkServer& link_server)
				: comm(comm), termination(termination), mutex_server(mutex_server), link_server(link_server) {
					setEpoch(Epoch::EVEN);
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
				termination.terminate(*this);
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
			void waitSendRequest(MPI_Request* req) {
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "wait for send...");
				bool sent = comm.test(req);

				while(!sent) {
					handleIncomingRequests();
					sent = comm.test(req);
				}
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "Request sent.");
			}

			/**
			 * Method used to wait for request responses (to READ, ACQUIRE, LOCK
			 * or LOCK_SHARED) to be available.
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
			 * @param source rank of the process from which the response should
			 * be received
			 * @param tag response tag (READ_RESPONSE, ACQUIRE_RESPONSE, etc)
			 * @param status pointer to MPI_Status, passed to
			 * api::communication::MpiCommunicator::Iprobe
			 */
			void waitResponse(int source, api::Tag tag, MPI_Status* status) {
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "wait for response...");
				bool response_available = comm.Iprobe(source, getEpoch() | tag, status);

				while(!response_available) {
					handleIncomingRequests();
					response_available = comm.Iprobe(source, getEpoch() | tag, status);
				}
				FPMAS_LOGV(comm.getRank(), "SERVER_PACK", "Response available.");
			}
		};
}}}
#endif
