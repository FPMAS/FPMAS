#ifndef PROXY_H
#define PROXY

#include <unordered_map>

namespace FPMAS {
	namespace graph {
		namespace proxy {

			/**
			 * The proxy can be used to locate ANY nodes currently accessible
			 * from the associated DistributedGraph, including GhostNodes.
			 *
			 * The corresponding maps are maintained externally at the
			 * DistributedGraph scale, when nodes are imported and exported
			 * using Zoltan.
			 *
			 * The Proxy must be synchronized at each step to maintain and
			 * update the current location map using the Zoltan migrations
			 * functions.
			 */
			class Proxy {
				private:
					int localProc = 0;
					std::unordered_map<unsigned long, int> origins;
					std::unordered_map<unsigned long, int> currentLocations;

				public:
					void setLocalProc(int);
					void setOrigin(unsigned long, int);
					void setCurrentLocation(unsigned long, int);
					int getOrigin(unsigned long);
					int getCurrentLocation(unsigned long);
					void setLocal(unsigned long);


					// TODO : sync functions

			};
		}
	}
}

#endif
