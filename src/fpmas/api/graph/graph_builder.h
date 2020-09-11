#ifndef FPMAS_GRAPH_BUILDER_API_H
#define FPMAS_GRAPH_BUILDER_API_H

#include "distributed_graph.h"

namespace fpmas { namespace api { namespace graph {

	template<typename T>
	class GraphBuilder {
		public:
			virtual void build(
					std::vector<T> nodes_data,
					LayerId layer,
					DistributedGraph<T>& graph) = 0;
	};
}}}
#endif
