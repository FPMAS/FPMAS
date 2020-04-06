#ifndef MPI_COMMUNICATOR_API_H
#define MPI_COMMUNICATOR_API_H

#include "mpi.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::communication {

	/**
	 * Used to smartly manage message reception to prevent message mixing
	 * accross several time steps.
	 */
	enum Epoch : int {
		EVEN = 0x00,
		ODD = 0x10
	};


	/**
	 * Tags used to smartly manage MPI communications.
	 */
	enum Tag : int {
		READ = 0x00,
		READ_RESPONSE = 0x01,
		ACQUIRE = 0x02,
		ACQUIRE_RESPONSE = 0x03,
		ACQUIRE_GIVE_BACK = 0x04,
		TOKEN = 0x05,
		END = 0x06
	};

	/**
	 * Used by the termination algorithm.
	 */
	enum Color : int {
		WHITE = 0,
		BLACK = 1
	};

	/**
	 * MpiCommunicator interface.
	 *
	 * Defines operations that might be used by the RequestHandler.
	 */
	class MpiCommunicator {
		public:
			/**
			 * Returns the rank of this communicator.
			 * @return rank
			 */
			virtual int getRank() const = 0;
			/**
			 * Returns the size of the group to which this communicator
			 * belongs.
			 * @return group size
			 */
			virtual int getSize() const = 0;

			/**
			 * Sends a colored token to the destination proc.
			 *
			 * @param token token color
			 * @param destination destination rank
			 */
			virtual void send(Color token, int destination) = 0;

			/**
			 * Sends an END message to the destination proc.
			 *
			 * @param destination destination rank
			 */
			virtual void sendEnd(int destination) = 0;

			/**
			 * Asynchronous non-blocking send for a DistributedId.
			 *
			 * @param id id to send
			 * @param destination destination rank
			 * @param tag send tag
			 * @param req MPI request
			 */
			virtual void Issend(
					const DistributedId& id, int destination, int tag, MPI_Request* req) = 0;
			/**
			 * Synchronous non-blocking send for an std::string.
			 *
			 * @param str std::string to send
			 * @param destination destination rank
			 * @param tag send tag
			 * @param req MPI request
			 */
			virtual void Issend(
					const std::string& str, int destination, int tag, MPI_Request* req) = 0;

			/**
			 * Asynchronous non-blocking send for an std::string.
			 *
			 * @param str std::string to send
			 * @param destination destination rank
			 * @param tag send tag
			 * @param req MPI request
			 */
			virtual void Isend(
					const std::string&, int destination, int tag, MPI_Request*) = 0;

			/**
			 * Blocking probe.
			 *
			 * Blocks until a message with the specified tag is received from
			 * source.
			 *
			 * @param source source rank
			 * @param tag recv tag
			 * @param status MPI status
			 */
			virtual void probe(int source, int tag, MPI_Status* status) = 0;

			/**
			 * Non-blocking probe.
			 *
			 * Return 1 iff a message with the specified tag can be received
			 * from source.
			 *
			 * @param source source rank
			 * @param tag recv tag
			 * @param status MPI status
			 */
			virtual int Iprobe(int source, int tag, MPI_Status* status) = 0;

			/**
			 * Receives the message corresponding to status.
			 *
			 * status should have been passed to a previous probe request.
			 *
			 * @param status ready to receive MPI status
			 * @param data data output
			 */
			virtual void recv(MPI_Status* status, Color& data) = 0;

			/**
			 * Receives an END message.
			 *
			 * Completes an eventually synchronous send operation.
			 *
			 * status should have been passed to a previous probe request.
			 *
			 * @param status ready to receive MPI status
			 */
			virtual void recvEnd(MPI_Status* status) = 0;

			/**
			 * Receives the message corresponding to status.
			 *
			 * status should have been passed to a previous probe request.
			 *
			 * @param status ready to receive MPI status
			 * @param string string output
			 */
			virtual void recv(MPI_Status* status, std::string& string) = 0;

			/**
			 * Receives the message corresponding to status.
			 *
			 * status should have been passed to a previous probe request.
			 *
			 * @param status ready to receive MPI status
			 * @param id id output
			 */
			virtual void recv(MPI_Status* status, DistributedId& id) = 0;

			virtual ~MpiCommunicator() {};
	};
}
#endif
