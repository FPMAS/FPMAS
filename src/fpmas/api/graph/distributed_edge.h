#ifndef FPMAS_DISTRIBUTED_EDGE_API_H
#define FPMAS_DISTRIBUTED_EDGE_API_H

/** \file src/fpmas/api/graph/distributed_edge.h
 * DistributedEdge API
 */

#include "fpmas/api/graph/edge.h"
#include "distributed_id.h"
#include "location_state.h"

namespace fpmas {namespace api { namespace graph {
	template<typename> class DistributedNode;
	/**
	 * DistributedEdge API.
	 *
	 * The DistributedEdge is an extension of the Edge API, specialized using
	 * DistributedId and DistributedNode, and introduces some distribution
	 * related concepts.
	 *
	 * @tparam T associated node data type
	 */
	template<typename T>
		class DistributedEdge
		: public virtual fpmas::api::graph::Edge<DistributedId, DistributedNode<T>> {
			typedef fpmas::api::graph::Edge<DistributedId, DistributedNode<T>> EdgeBase;
			public:
			/**
			 * Current state of the edge.
			 *
			 * A DistributedEdge is \LOCAL iff its source and target nodes are
			 * \LOCAL.
			 *
			 * @return current edge state
			 */
			virtual LocationState state() const = 0;
			/**
			 * Updates the state of the edge.
			 *
			 * Only intended for internal / serialization usage.
			 *
			 * @param state new state
			 */
			virtual void setState(LocationState state) = 0;

			virtual ~DistributedEdge() {}
		};
}}}
#endif
