#ifndef READERS_WRITERS_H
#define READERS_WRITERS_H

#include <set>
#include <vector>
#include <unordered_map>
#include <queue>
#include "utils/macros.h"

using FPMAS::graph::base::NodeId;

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
			NodeId id;
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
			ReadersWriters(NodeId id, SyncMpiCommunicator& comm) : id(id), comm(comm) {}

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
			mutable std::unordered_map<NodeId, ReadersWriters> readersWriters;

			void lock(NodeId id);
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
			void read(NodeId id, int destination);

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
			void write(NodeId id, int destination);

			/**
			 * Releases the locked resource, and respond to pending requests.
			 *
			 * @param id resource id
			 */
			void release(NodeId id);

			/**
			 * Returns a const reference to the ReadersWriters instance
			 * currently managing the specified resource.
			 */
			const ReadersWriters& get(NodeId id) const;

	};

}
#endif
