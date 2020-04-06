#ifndef SYNC_COMMUNICATION_API_H
#define SYNC_COMMUNICATION_API_H

#include <string>
#include "communication.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::communication {

	/**
	 * Interface that defines operations used to perform read and acquire
	 * requests.
	 */
	class RequestHandler {
		public:
			/**
			 * Performs a read request of the data represented by `id` on the
			 * specified `location`.
			 *
			 * @param id id of the data to read
			 * @param location rank of the proc where the data is located
			 * @return serialized data
			 */
			virtual std::string read(DistributedId id, int location) = 0;

			virtual void respondToRead(int, DistributedId) = 0;

			/**
			 * Performs an acquire request of the data represented by `id` on the
			 * specified `location`.
			 *
			 * Upon return, the corresponding data should be locked until it is
			 * given back.
			 *
			 * @param id id of the data to read
			 * @param location rank of the proc where the data is located
			 * @return serialized data
			 */
			virtual std::string acquire(DistributedId, int) = 0;

			virtual void respondToAcquire(int, DistributedId) = 0;

			/**
			 * Gives back and updates the data represented by `id` to its
			 * `origin` proc.
			 *
			 * @param id id of the acquired data
			 * @param origin rank of the proc where the data come from
			 */
			virtual void giveBack(DistributedId, int) = 0;

			/**
			 * Enters a passive mode and waits for other procs to finish.
			 *
			 * Blocks until all other procs also have entered to passive mode.
			 * When a process becomes passive, it should still be able to
			 * respond to others read and acquire requests, to allow them to
			 * `terminate` their own process.
			 */
			virtual void terminate() = 0;

			/**
			 * Returns a reference to the internal MpiCommunicator.
			 *
			 * @return reference to the internal MpiCommunicator instance
			 */
			virtual const MpiCommunicator& getMpiComm() = 0;

			virtual ~RequestHandler() {};
	};
}
#endif
