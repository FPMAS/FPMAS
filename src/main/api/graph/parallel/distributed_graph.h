#ifndef DISTRIBUTED_GRAPH_API_H
#define DISTRIBUTED_GRAPH_API_H

#include "api/communication/communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/location_manager.h"
#include "api/graph/parallel/distributed_node.h"
#include "api/graph/parallel/synchro/sync_mode.h"
#include "api/load_balancing//load_balancing.h"

#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename T>
	class DistributedGraph 
		: public virtual FPMAS::api::graph::base::Graph<DistributedNode<T>, DistributedArc<T>> {
		public:
			typedef FPMAS::api::graph::base::Graph<DistributedNode<T>, DistributedArc<T>> GraphBase;

			using typename GraphBase::LayerIdType;
			using typename GraphBase::NodeMap;
			using typename GraphBase::ArcMap;

			typedef typename api::load_balancing::LoadBalancing<DistributedNode<T>>::PartitionMap PartitionMap;

		public:
			virtual const LocationManager<T>& getLocationManager() const = 0;

			virtual DistributedNode<T>* importNode(DistributedNode<T>*) = 0;
			virtual DistributedArc<T>* importArc(DistributedArc<T>*) = 0;

			virtual DistributedNode<T>* buildNode(const T&) = 0;
			virtual DistributedNode<T>* buildNode(T&&) = 0;

			virtual DistributedArc<T>* link(DistributedNode<T>*, DistributedNode<T>*, LayerIdType) = 0;

			virtual void clear(DistributedArc<T>*) = 0;
			virtual void clear(DistributedNode<T>*) = 0;

			virtual void balance(api::load_balancing::LoadBalancing<T>&) = 0;
			virtual void distribute(PartitionMap partition) = 0;
			virtual void synchronize() = 0;
	};
}
#endif
