#ifndef DISTRIBUTED_NODE_API_H
#define DISTRIBUTED_NODE_API_H

#include "api/graph/base/node.h"
#include "api/graph/parallel/distributed_arc.h"
#include "api/graph/parallel/synchro/mutex.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::graph::parallel {

	template<typename T, typename DistArcImpl>
	class DistributedNode 
		: public virtual FPMAS::api::graph::base::Node<T, DistributedId, DistArcImpl> {
		typedef FPMAS::api::graph::base::Node<T, DistributedId, DistArcImpl> node_base;
		public:
			//using typename node_base::arc_type;
			virtual int getLocation() const = 0;
			virtual void setLocation(int) = 0;

			virtual LocationState state() const = 0;
			virtual void setState(LocationState state) = 0;

			virtual synchro::Mutex<T>& mutex() = 0;
	};

}

#endif
