#ifndef LOCATION_MANAGER_API_H
#define LOCATION_MANAGER_API_H

#include "api/graph/base/id.h"
#include "graph/parallel/distributed_id.h"
#include "api/graph/parallel/distributed_node.h"

namespace fpmas::api::graph::parallel {

	template<typename T>
	class LocationManager {
		public:
			typedef api::graph::parallel::DistributedNode<T> DistNode;
			typedef 
				std::unordered_map<DistributedId, DistNode*, fpmas::api::graph::base::IdHash<DistributedId>>
				NodeMap;

			virtual void addManagedNode(DistNode*, int initialLocation) = 0;
			virtual void removeManagedNode(DistNode*) = 0;

			virtual const NodeMap& getLocalNodes() const = 0;
			virtual const NodeMap& getDistantNodes() const = 0;
			virtual void setLocal(DistNode* node) = 0;
			virtual void setDistant(DistNode* node) = 0;
			virtual void remove(DistNode* node) = 0;

			virtual void updateLocations(
				const NodeMap& localNodesToUpdate
				) = 0;
	};
}

#endif
