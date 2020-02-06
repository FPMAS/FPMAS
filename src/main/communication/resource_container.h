#ifndef RESOURCE_HANDLER_H
#define RESOURCE_HANDLER_H

#include <string>

namespace FPMAS {
	namespace communication {
		/**
		 * An abstract class that must be implemented to provide application
		 * dependent serialized data to the TerminableMpiCommunicator.
		 */
		class ResourceContainer {
			public:
				/**
				 * Returns serialized data that correspond to the specified id.
				 *
				 * @param id data id
				 * @return serialized data
				 */
				virtual std::string getLocalData(unsigned long id) const = 0;

				/**
				 * Returns serialized data that correspond to updates locally
				 * applied to the data corresponding to the specified id so
				 * that it can be returned to the owner proc.
				 *
				 * @param id data id
				 * @return serialized updated data
				 */
				virtual std::string getUpdatedData(unsigned long id) const = 0;

				/**
				 * Updates local data corresponding to id from the specified
				 * serialized updated data.
				 *
				 * Typically, a proc that acquired the data will serialize the
				 * updates with getUpdatedData() once it releases it, and then
				 * send it to the host proc that will update the data with
				 * updateData().
				 */
				virtual void updateData(unsigned long id, std::string data) = 0;
		};
	}
}

#endif
