#ifndef DISTRIBUTED_ARC_API
#define DISTRIBUTED_ARC_API

#include "api/graph/base/arc.h"
#include "graph/parallel/distributed_id.h"
#include "location_state.h"

namespace FPMAS::api::graph::parallel {
	template<typename T> class DistributedNode;

	template<typename T>
		class DistributedArc
		: public virtual FPMAS::api::graph::base::Arc<DistributedId, DistributedNode<T>> {
			typedef FPMAS::api::graph::base::Arc<DistributedId, DistributedNode<T>> arc_base;
			public:
				using typename arc_base::node_type;
				using typename arc_base::layer_id_type;

				virtual LocationState state() const = 0;
				virtual void setState(LocationState state) = 0;
		};
};

#endif
