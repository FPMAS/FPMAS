#ifndef READERS_WRITERS_H
#define READERS_WRITERS_H

#include <set>
#include <vector>
#include <unordered_map>
#include <queue>
#include "utils/macros.h"
#include "graph/parallel/distributed_id.h"

using FPMAS::graph::parallel::DistributedId;

namespace FPMAS::communication {
	class ResourceManager;
	class SyncMpiCommunicator;

	/**
	 * Class managing concurrent read / write accesses to the resource
	 * represented by the id.
	 */
	class ReadersWriters {
		friend ResourceManager;
		private:
			DistributedId id;
			std::queue<int> read_requests;
			std::queue<int> write_requests;
			SyncMpiCommunicator& comm;
			bool locked = false;
			void lock();

		public:
			/**
			 * ReadersWriters constructor.
			 *
			 * @param id resource id
			 * @param comm reference to the MPI communicator
			 */
			ReadersWriters(DistributedId id, SyncMpiCommunicator& comm) : id(id), comm(comm) {}

			/**
			 * Performs a read operation on the local resource for the
			 * specified destination.
			 *
			 * The request is put in a waiting queue if the resource is
			 * currently locked.
			 *
			 * @param destination destination proc
			 */
			void read(int destination);

			/**
			 * Performs a write operation on the local resource for the
			 * specified destination.
			 *
			 * The request is put in a waiting queue if the resource is
			 * currently locked.
			 *
			 * On response, the resource is acquired by the destination that
			 * must later give it back to release() the resource.
			 *
			 * @param destination destination proc
			 */
			void write(int destination);

			/**
			 * Releases the locked resource, and respond to pending requests.
			 */
			void release();

			/**
			 * Checks if the local resource is locked.
			 *
			 * @return true iff the resource is locked
			 */
			bool isLocked() const;

	};

	/**
	 * Class managing concurrent read / write access to a set of resources,
	 * each represented by a given id.
	 */
	class ResourceManager {
		friend SyncMpiCommunicator;
		private:
			SyncMpiCommunicator& comm;
			mutable std::unordered_map<DistributedId, ReadersWriters> readersWriters;

			void lock(DistributedId id);
			void clear();

		public:
			/**
			 * ResourceManager constructor.
			 *
			 * @param comm reference to the MPI communicator
			 */
			ResourceManager(SyncMpiCommunicator& comm) : comm(comm) {};

			/**
			 * Performs a read operation on the specified resource for the
			 * specified destination.
			 *
			 * The request is put in a waiting queue if the resource is
			 * currently locked.
			 *
			 * @param id resource id
			 * @param destination destination proc
			 */
			void read(DistributedId id, int destination);

			/**
			 * Performs a write operation on the local resource for the
			 * specified destination.
			 *
			 * The request is put in a waiting queue if the resource is
			 * currently locked.
			 *
			 * On response, the resource is acquired by the destination that
			 * must later give it back to release() the resource.
			 *
			 * @param id resource id
			 * @param destination destination proc
			 */
			void write(DistributedId id, int destination);

			/**
			 * Releases the locked resource, and respond to pending requests.
			 *
			 * @param id resource id
			 */
			void release(DistributedId id);

			/**
			 * Returns a const reference to the ReadersWriters instance
			 * currently managing the specified resource.
			 */
			const ReadersWriters& get(DistributedId id) const;

	};

}
#endif
