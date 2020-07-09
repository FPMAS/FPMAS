#ifndef DISTRIBUTED_EDGE_API_H
#define DISTRIBUTED_EDGE_API_H

#include "fpmas/api/graph/edge.h"
#include "distributed_id.h"
#include "location_state.h"

namespace fpmas {namespace api { namespace graph {
	template<typename> class DistributedNode;

	template<typename T>
		class DistributedEdge
		: public virtual fpmas::api::graph::Edge<DistributedId, DistributedNode<T>> {
			typedef fpmas::api::graph::Edge<DistributedId, DistributedNode<T>> EdgeBase;
			public:
				using typename EdgeBase::NodeType;
				using typename EdgeBase::LayerIdType;

				virtual LocationState state() const = 0;
				virtual void setState(LocationState state) = 0;

				virtual ~DistributedEdge() {}
		};
}}}
#endif
