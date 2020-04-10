#ifndef PROXY_API_H
#define PROXY_API_H

#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename IdType>
	class Proxy {
		public:
			virtual void setOrigin(IdType id, int rank) = 0;
			virtual int getOrigin(IdType id) const = 0;

			virtual void setCurrentLocation(IdType id, int rank) = 0;
			virtual int getCurrentLocation(IdType id) const = 0;

			virtual void synchronize() = 0;

	};
}
#endif
