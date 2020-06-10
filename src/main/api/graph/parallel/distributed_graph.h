#ifndef DISTRIBUTED_GRAPH_API_H
#define DISTRIBUTED_GRAPH_API_H

#include "api/communication/communication.h"
#include "api/graph/base/graph.h"
#include "api/graph/parallel/load_balancing.h"
#include "api/graph/parallel/location_manager.h"
#include "api/graph/parallel/distributed_node.h"
#include "api/graph/parallel/synchro/sync_mode.h"

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

			typedef typename LoadBalancing<DistributedNode<T>>::PartitionMap PartitionMap;

		public:
			virtual const LocationManager<T>& getLocationManager() const = 0;

			virtual DistributedNode<T>* importNode(DistributedNode<T>*) = 0;
			virtual DistributedArc<T>* importArc(DistributedArc<T>*) = 0;

			virtual DistributedNode<T>* buildNode(const T&) = 0;
			virtual DistributedNode<T>* buildNode(T&&) = 0;

			virtual DistributedArc<T>* link(DistributedNode<T>*, DistributedNode<T>*, LayerIdType) = 0;

			virtual void clear(DistributedArc<T>*) = 0;
			virtual void clear(DistributedNode<T>*) = 0;

			virtual void balance(api::graph::parallel::LoadBalancing<T>&) = 0;
			virtual void distribute(PartitionMap partition) = 0;
			virtual void synchronize() = 0;
	};

/*
 *    template<typename NodeType, typename Graph, typename... Args>
 *        NodeType* buildNode(Graph& graph, Args... args) {
 *            NodeType* node = new NodeType(
 *                    graph.nodeId()++,
 *                    std::forward<Args>(args)...
 *                    );
 *            graph.insert(node);
 *            graph.getLocationManager().setLocal(node);
 *            graph.getLocationManager().addManagedNode(node, graph.getMpiCommunicator().getRank());
 *            node->setMutex(graph.getSyncModeRuntime().buildMutex(node->getId(), node->data()));
 *
 *            return node;
 *        }
 */
}

#endif
