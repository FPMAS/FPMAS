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
				virtual std::string getUpdatedData(unsigned long id) const = 0;
				virtual void updateData(unsigned long id, std::string data) = 0;
		};
	}
}

#endif
