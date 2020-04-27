#ifndef MOCK_GRAPH_BASE_H
#define MOCK_GRAPH_BASE_H
#include "gmock/gmock.h"

#include "graph/base/basic_graph.h"

template<typename NodeType, typename ArcType>
class MockGraphBase : 
	public virtual FPMAS::graph::base::AbstractGraphBase<NodeType, ArcType> {
		typedef FPMAS::graph::base::AbstractGraphBase<NodeType, ArcType>
		graph_base;
		using typename graph_base::node_type;
		using typename graph_base::arc_type;

		public:
			MOCK_METHOD(void, removeNode, (node_type*), (override));
			MOCK_METHOD(void, unlink, (arc_type*), (override));

};
#endif
