#ifndef MOCK_GRAPH_BASE_H
#define MOCK_GRAPH_BASE_H
#include "gmock/gmock.h"

#include "fpmas/api/graph/graph.h"

template<typename NodeType, typename EdgeType>
class MockGraph : 
	public virtual fpmas::api::graph::Graph<NodeType, EdgeType> {
		typedef fpmas::api::graph::Graph<NodeType, EdgeType>
		GraphBase;
		using typename GraphBase::NodeIdType;
		using typename GraphBase::NodeMap;

		using typename GraphBase::EdgeIdType;
		using typename GraphBase::EdgeMap;

		public:
			MOCK_METHOD(void, insert, (NodeType*), (override));
			MOCK_METHOD(void, insert, (EdgeType*), (override));

			MOCK_METHOD(void, erase, (NodeType*), (override));
			MOCK_METHOD(void, erase, (EdgeType*), (override));

			MOCK_METHOD(void, addCallOnInsertNode, (fpmas::api::utils::Callback<NodeType*>*), (override));
			MOCK_METHOD(void, addCallOnEraseNode, (fpmas::api::utils::Callback<NodeType*>*), (override));
			MOCK_METHOD(void, addCallOnInsertEdge, (fpmas::api::utils::Callback<EdgeType*>*), (override));
			MOCK_METHOD(void, addCallOnEraseEdge, (fpmas::api::utils::Callback<EdgeType*>*), (override));

			MOCK_METHOD(NodeType*, getNode, (NodeIdType), (override));
			MOCK_METHOD(const NodeType*, getNode, (NodeIdType), (const, override));
			MOCK_METHOD(const NodeMap&, getNodes, (), (const, override));

			MOCK_METHOD(EdgeType*, getEdge, (EdgeIdType), (override));
			MOCK_METHOD(const EdgeType*, getEdge, (EdgeIdType), (const, override));
			MOCK_METHOD(const EdgeMap&, getEdges, (), (const, override));

			MOCK_METHOD(void, clear, (), (override));
};
#endif
