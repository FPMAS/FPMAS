#ifndef RESOURCE_HANDLER_H
#define RESOURCE_HANDLER_H

#include <string>

namespace FPMAS {
	namespace communication {
		class ResourceManager {
			public:
				virtual std::string getResource(unsigned long) = 0;
		};
	}
}

#endif
