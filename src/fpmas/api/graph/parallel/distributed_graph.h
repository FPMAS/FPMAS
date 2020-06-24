#ifndef DISTRIBUTED_GRAPH_API_H
#define DISTRIBUTED_GRAPH_API_H

#include "fpmas/api/communication/communication.h"
#include "fpmas/api/graph/base/graph.h"
#include "fpmas/api/graph/parallel/location_manager.h"
#include "fpmas/api/graph/parallel/distributed_node.h"
#include "fpmas/api/load_balancing/load_balancing.h"
#include "fpmas/api/synchro/sync_mode.h"

#include "distributed_id.h"

namespace fpmas::api::graph::parallel {

	template<typename T>
	class DistributedGraph 
		: public virtual fpmas::api::graph::base::Graph<DistributedNode<T>, DistributedArc<T>> {
		public:
			typedef fpmas::api::graph::base::Graph<DistributedNode<T>, DistributedArc<T>> GraphBase;

			using typename GraphBase::LayerIdType;
			using typename GraphBase::NodeMap;
			using typename GraphBase::ArcMap;

			typedef typename api::load_balancing::LoadBalancing<DistributedNode<T>>::PartitionMap PartitionMap;
			typedef api::utils::Callback<DistributedNode<T>*> NodeCallback;

		public:
			virtual const api::communication::MpiCommunicator& getMpiCommunicator() const = 0;
			virtual const LocationManager<T>& getLocationManager() const = 0;

			virtual DistributedNode<T>* importNode(DistributedNode<T>*) = 0;
			virtual DistributedArc<T>* importArc(DistributedArc<T>*) = 0;

			//virtual DistributedNode<T>* buildNode(const T&) = 0;
			virtual DistributedNode<T>* buildNode(T&&) = 0;

			virtual void addCallOnSetLocal(NodeCallback*) = 0;
			virtual void addCallOnSetDistant(NodeCallback*) = 0;

			virtual DistributedArc<T>* link(DistributedNode<T>*, DistributedNode<T>*, LayerIdType) = 0;

			virtual void clearArc(DistributedArc<T>*) = 0;
			virtual void clearNode(DistributedNode<T>*) = 0;

			virtual void balance(api::load_balancing::LoadBalancing<T>&) = 0;
			virtual void balance(api::load_balancing::FixedVerticesLoadBalancing<T>&, PartitionMap fixed_nodes) = 0;
			virtual void distribute(PartitionMap partition) = 0;
			virtual void synchronize() = 0;
	};
}
#endif
