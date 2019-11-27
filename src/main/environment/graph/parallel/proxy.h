#ifndef PROXY_H
#define PROXY

#include <unordered_map>

namespace FPMAS {
	namespace graph {
		namespace proxy {

			class Proxy {
				private:
					int localProc = 0;
					std::unordered_map<unsigned long, int> origins;
					std::unordered_map<unsigned long, int> currentLocations;

				public:
					void setLocalProc(int);
					void setOrigin(unsigned long, int);
					int getOrigin(unsigned long);
					int getCurrentLocation(unsigned long);


					// TODO : sync functions

			};
		}
	}
}

#endif
