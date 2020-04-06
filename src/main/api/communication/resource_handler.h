#ifndef RESOURCE_HANDLER_H
#define RESOURCE_HANDLER_H

#include <string>
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::communication {
	/**
	 * An abstract class that must be implemented to provide application
	 * dependent serialized data to the TerminableMpiCommunicator.
	 */
	class ResourceHandler {
		public:
			/**
			 * Returns serialized data that correspond to the specified id.
			 *
			 * @param id data id
			 * @return serialized data
			 */
			virtual std::string getLocalData(DistributedId id) const = 0;

			/**
			 * Returns serialized data that correspond to updates locally
			 * applied to the data corresponding to the specified id so
			 * that it can be returned to the owner proc.
			 *
			 * @param id data id
			 * @return serialized updated data
			 */
			virtual std::string getUpdatedData(DistributedId id) const = 0;

			/**
			 * Updates local data corresponding to id from the specified
			 * serialized updated data.
			 *
			 * Typically, a proc that acquired the data will serialize the
			 * updates with getUpdatedData() once it releases it, and then
			 * send it to the host proc that will update the data with
			 * updateData().
			 */
			virtual void updateData(DistributedId id, std::string data) = 0;

			virtual ~ResourceHandler() {};
	};
}

#endif
