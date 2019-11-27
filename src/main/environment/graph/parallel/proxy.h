#ifndef PROXY_H
#define PROXY

#include <unordered_map>

namespace FPMAS {
	namespace graph {
		namespace proxy {

			class Proxy {
				private:
					std::unordered_map<unsigned long, int> origins;

					std::unordered_map<unsigned long, int> currentLocations;

				public:
					void setOrigin(unsigned long, int);
					int getOrigin(unsigned long);
					int getCurrentLocation(unsigned long);


					// TODO : sync functions

			};
		}
	}
}

#endif
