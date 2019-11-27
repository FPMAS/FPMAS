#ifndef PROXY_H
#define PROXY

#include <unordered_map>

namespace FPMAS {
	namespace graph {
		namespace proxy {
			class Location {
				public:
					int proc;
					int part;
					Location();
					Location(int, int);
			};

			class Proxy {
				private:
					std::unordered_map<unsigned long, Location> origins;

					std::unordered_map<unsigned long, Location> currentLocations;

				public:
					void setOrigin(unsigned long, int, int);
					Location getOrigin(unsigned long);
					Location getCurrentLocation(unsigned long);


					// TODO : sync functions

			};
		}
	}
}

#endif
