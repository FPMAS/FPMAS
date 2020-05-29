#ifndef DISTRIBUTED_ARC_API_H
#define DISTRIBUTED_ARC_API_H

#include "api/graph/base/arc.h"
#include "graph/parallel/distributed_id.h"
#include "location_state.h"

namespace FPMAS::api::graph::parallel {
	template<typename, typename> class DistributedNode;

	template<typename T, typename DistNodeImpl>
		class DistributedArc
		: public virtual FPMAS::api::graph::base::Arc<DistributedId, DistNodeImpl> {
			typedef FPMAS::api::graph::base::Arc<DistributedId, DistNodeImpl> ArcBase;
			public:
				using typename ArcBase::NodeType;
				using typename ArcBase::LayerIdType;

				virtual LocationState state() const = 0;
				virtual void setState(LocationState state) = 0;
		};
}
#endif
