#ifndef MOCK_GRAPH_BASE_H
#define MOCK_GRAPH_BASE_H
#include "gmock/gmock.h"

#include "api/graph/base/graph.h"

template<typename NodeType, typename ArcType>
class MockGraphBase : 
	public virtual FPMAS::api::graph::base::Graph<NodeType, ArcType> {
		typedef FPMAS::api::graph::base::Graph<NodeType, ArcType>
		graph_base;
		using typename graph_base::node_type;
		using typename graph_base::node_id_type;
		using typename graph_base::node_map;

		using typename graph_base::arc_type;
		using typename graph_base::arc_id_type;
		using typename graph_base::arc_map;

		public:
			MOCK_METHOD(void, insert, (node_type*), (override));
			MOCK_METHOD(void, insert, (arc_type*), (override));

			MOCK_METHOD(void, erase, (node_type*), (override));
			MOCK_METHOD(void, erase, (arc_type*), (override));

			MOCK_METHOD(const node_id_type&, currentNodeId, (), (const, override));
			MOCK_METHOD(node_type*, getNode, (node_id_type), (override));
			MOCK_METHOD(const node_type*, getNode, (node_id_type), (const, override));
			MOCK_METHOD(const node_map&, getNodes, (), (const, override));

			MOCK_METHOD(const arc_id_type&, currentArcId, (), (const, override));
			MOCK_METHOD(arc_type*, getArc, (arc_id_type), (override));
			MOCK_METHOD(const arc_type*, getArc, (arc_id_type), (const, override));
			MOCK_METHOD(const arc_map&, getArcs, (), (const, override));

			MOCK_METHOD(void, removeNode, (node_type*), (override));
			MOCK_METHOD(void, unlink, (arc_type*), (override));

};
#endif
