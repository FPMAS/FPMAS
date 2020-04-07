#ifndef READERS_WRITERS_API_H
#define READERS_WRITERS_API_H

#include "graph/parallel/distributed_id.h"
#include "request_handler.h"

namespace FPMAS::api::communication {

	/**
	 * ReadersWriters algorithm interface.
	 *
	 * The implemented algorithm should fulfill the following requirements :
	 * - any number of read operations can be processed simultaneously
	 * - if the data has been `acquired`, subsequent `read` and `acquire`
	 *   requests are delayed until the data is `released`.
	 */
	class ReadersWriters {
		public:
			/**
			 * Perform a read request to the underlying data.
			 *
			 * @param rank rank of the reading proc
			 */
			virtual void read(int rank) = 0;

			/**
			 * Performs an acquire request to the underlying data.
			 *
			 * @param rank rank of the acquiring process
			 */
			virtual void acquire(int rank) = 0;

			/**
			 * Releases the previously acquired data.
			 */
			virtual void release() = 0;

			/**
			 * Locks data.
			 */
			virtual void lock() = 0;

			/**
			 * Returns true iff the underlying data is locked.
			 *
			 * Data should be locked iff it has been `acquired` but not
			 * `released` yet.
			 *
			 * @return locked
			 */
			virtual bool isLocked() const = 0;

	};

	/**
	 * Manages a set of resources, preserving consistent read/write accesses
	 * thanks to the specified `ReadersWritersType`.
	 *
	 * @tparam ReadersWritersType ReadersWriters implementation
	 */
	template<typename ReadersWritersType>
	class ReadersWritersManager {
		protected:
			/**
			 * RequestHandler used to transmit requests to other procs.
			 */
			RequestHandler& requestHandler;

		public:
			/**
			 * ReadersWritersManager constructor.
			 */
			ReadersWritersManager(RequestHandler& requestHandler) 
				: requestHandler(requestHandler) {}

			/**
			 * Accesses the `ReadersWritersType` instance currently managy the
			 * data with the specified `id`.
			 *
			 * If this function has been already been called with the given id, 
			 * the `ReadersWritersType` instance currently used should be
			 * returned.
			 * Else, a new `ReadersWritersType` instance should be initialized
			 * and returned.
			 *
			 * @param id data id
			 * @return reference to `ReadersWritersType` instance currently
			 * managing data with the given `id`
			 */
			virtual ReadersWritersType& operator[](const DistributedId& id) = 0;

			/**
			 * Re-initializes all the `ReadersWritersManager` entries.
			 *
			 * Typically called when it is guaranted that no pending request
			 * left in the global execution, in order to free some memory.
			 */
			// TODO : might return exception if data not released when called?
			virtual void clear() = 0;
	};
}
#endif
