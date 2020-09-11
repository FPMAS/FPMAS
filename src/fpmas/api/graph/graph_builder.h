#ifndef FPMAS_GRAPH_BUILDER_API_H
#define FPMAS_GRAPH_BUILDER_API_H

#include "distributed_graph.h"

namespace fpmas { namespace api { namespace graph {

	template<typename T>
		class NodeBuilder {
			public:
				virtual std::size_t nodeCount() = 0;
				virtual DistributedNode<T>* buildNode(DistributedGraph<T>& graph) = 0;
		};

	template<typename T>
	class GraphBuilder {
		public:
			virtual void build(
					NodeBuilder<T>& node_builder,
					LayerId layer,
					DistributedGraph<T>& graph) = 0;
	};
}}}
#endif
