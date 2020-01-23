#ifndef RESOURCE_HANDLER_H
#define RESOURCE_HANDLER_H

#include <string>

namespace FPMAS {
	namespace communication {
		/**
		 * An abstract class that must be implemented to provide application
		 * dependent serialized data to the TerminableMpiCommunicator.
		 */
		class ResourceManager {
			public:
				/**
				 * Returns serialized data that correspond to the specified id.
				 *
				 * @param id data id
				 * @return serialized data
				 */
				virtual std::string getResource(unsigned long id) const = 0;
		};
	}
}

#endif
