#ifndef READERS_WRITERS_H
#define READERS_WRITERS_H

#include <set>
#include <vector>
#include <unordered_map>
#include <queue>
#include "utils/macros.h"
#include "graph/parallel/distributed_id.h"
#include "api/communication/request_handler.h"
#include "api/communication/readers_writers.h"

using FPMAS::graph::parallel::DistributedId;

namespace FPMAS::communication {

	/**
	 * Implementation of the "first ReadersWriters"
	 * problem[https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem]
	 * that gives preference to readers.
	 */
	class FirstReadersWriters : public api::communication::ReadersWriters {
		private:
			typedef api::communication::RequestHandler request_handler;

			DistributedId id;
			const int rank;
			api::communication::RequestHandler& requestHandler;
			std::queue<int> read_requests;
			std::queue<int> write_requests;
			bool locked = false;

		public:
			/**
			 * ReadersWriters constructor.
			 *
			 * @param id resource id
			 * @param requestHandler reference to the requestHandler used to
			 * transmit requests to other procs
			 * @param rank current MPI rank (only used for log purpose)
			 */
			FirstReadersWriters(DistributedId id, request_handler& requestHandler, int rank)
				: id(id), requestHandler(requestHandler), rank(rank) {}

			/**
			 * Locks the resource.
			 */
			void lock() override;

			/**
			 * Performs a read operation on the local resource for the
			 * specified destination.
			 *
			 * The request is put in a waiting queue if the resource is
			 * currently locked.
			 *
			 * @param destination destination proc
			 */
			void read(int destination) override;

			/**
			 * Performs an acquire operation on the local resource for the
			 * specified destination.
			 *
			 * The request is put in a waiting queue if the resource is
			 * currently locked.
			 *
			 * @param destination destination proc
			 */
			void acquire(int destination) override;

			/**
			 * Releases the locked resource, and respond to pending requests.
			 *
			 * First, unlock the resource respond to all pending read requests.
			 * Then, if at least one acquire request is pending, respond to the
			 * first acquire request.
			 *
			 * In any case, pending request are processed in the order they
			 * initially arrived.
			 */
			void release() override;

			/**
			 * Checks if the local resource is locked.
			 *
			 * @return true iff the resource is locked
			 */
			bool isLocked() const override;

	};

	/**
	 * HashMap based ReadersWritersManager implementation.
	 *
	 * @tparam ReadersWritersType ReadersWriters implementation
	 */
	template<class ReadersWritersType>
	class ReadersWritersManager : public api::communication::ReadersWritersManager<ReadersWritersType> {
		private:
			const int rank;
			std::unordered_map<DistributedId, ReadersWritersType> map;

		public:
			/**
			 * ReadersWritersManager constructor.
			 *
			 * @param requestHandler RequestHandler used to transmit requests
			 * to other procs
			 * @param current MPI rank (used only for log purpose)
			 */
			ReadersWritersManager(api::communication::RequestHandler& requestHandler, int rank)
				: api::communication::ReadersWritersManager<ReadersWritersType>(requestHandler), rank(rank) {}

			ReadersWritersType& operator[](const DistributedId& id) {
				try {
					return map.at(id);
				} catch (std::out_of_range) {
					map.insert(std::make_pair(id, ReadersWritersType(id, this->requestHandler, rank)));
					return map.at(id);
				}
			}

			void clear() {
				map.clear();
			}
	};
	/**
	 * Class managing concurrent read / write access to a set of resources,
	 * each represented by a given id.
	 */
   /* class ResourceManager {*/
		//private:
			//typedef api::communication::RequestHandler request_handler;
			//request_handler& comm;
			//mutable std::unordered_map<DistributedId, ReadersWriters> readersWriters;

		//public:
			/**
			 * ResourceManager constructor.
			 *
			 * @param comm reference to the MPI communicator
			 */
			//ResourceManager(request_handler& comm) : comm(comm) {};

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
			//void read(DistributedId id, int destination);

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
			//void write(DistributedId id, int destination);

			/**
			 * Releases the locked resource, and respond to pending requests.
			 *
			 * @param id resource id
			 */
			//void release(DistributedId id);

			/**
			 * Returns a const reference to the ReadersWriters instance
			 * currently managing the specified resource.
			 */
			//const ReadersWriters& get(DistributedId id) const;

			//void lock(DistributedId id);

			//void clear();
	/*};*/

}
#endif
