#ifndef DISTRIBUTED_ARC_API_H
#define DISTRIBUTED_ARC_API_H

#include "api/graph/base/arc.h"
#include "graph/parallel/distributed_id.h"
#include "location_state.h"

namespace fpmas::api::graph::parallel {
	template<typename> class DistributedNode;

	template<typename T>
		class DistributedArc
		: public virtual fpmas::api::graph::base::Arc<DistributedId, DistributedNode<T>> {
			typedef fpmas::api::graph::base::Arc<DistributedId, DistributedNode<T>> ArcBase;
			public:
				using typename ArcBase::NodeType;
				using typename ArcBase::LayerIdType;

				virtual LocationState state() const = 0;
				virtual void setState(LocationState state) = 0;

				virtual ~DistributedArc() {}
		};
}
#endif
