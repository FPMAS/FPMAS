#ifndef DISTRIBUTED_GRAPH_API_H
#define DISTRIBUTED_GRAPH_API_H

//#include "api/communication/communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/location_manager.h"
#include "api/graph/parallel/distributed_node.h"

#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename T>
	class DistributedGraph 
		: public virtual FPMAS::api::graph::base::Graph<DistributedNode<T>, DistributedArc<T>> {
		public:
			typedef FPMAS::api::graph::base::Graph<DistributedNode<T>, DistributedArc<T>> GraphBase;
			typedef DistributedNode<T> NodeType;
			typedef DistributedArc<T> ArcType;

			using typename GraphBase::LayerIdType;
			using typename GraphBase::NodeMap;
			using typename GraphBase::ArcMap;

			typedef typename LoadBalancing<DistributedNode<T>>::PartitionMap PartitionMap;

		public:
			virtual const LocationManager<DistributedNode<T>>& getLocationManager() const = 0;

			virtual NodeType* importNode(const NodeType&) = 0;
			virtual ArcType* importArc(const ArcType&) = 0;

			virtual void clear(ArcType*) = 0;
			virtual void clear(NodeType*) = 0;

			virtual void balance() = 0;
			virtual void distribute(PartitionMap partition) = 0;
			virtual void synchronize() = 0;
	};
}

#endif
