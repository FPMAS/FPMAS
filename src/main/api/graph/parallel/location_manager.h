#ifndef LOCATION_MANAGER_API_H
#define LOCATION_MANAGER_API_H

#include "api/graph/base/id.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename DistNode>
	class LocationManager {
		public:
			typedef 
				std::unordered_map<DistributedId, DistNode*, FPMAS::api::graph::base::IdHash<DistributedId>>
				node_map;
			virtual void addManagedNode(DistNode*, int initialLocation) = 0;
			virtual void removeManagedNode(DistNode*) = 0;

			virtual void updateLocations(
				node_map localNodes,
				node_map distantNodes
				) = 0;
	};
}

#endif
