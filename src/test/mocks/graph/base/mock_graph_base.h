#ifndef MOCK_GRAPH_BASE_H
#define MOCK_GRAPH_BASE_H
#include "gmock/gmock.h"

#include "api/graph/base/graph.h"

template<typename NodeType, typename ArcType>
class MockGraph : 
	public virtual FPMAS::api::graph::base::Graph<NodeType, ArcType> {
		typedef FPMAS::api::graph::base::Graph<NodeType, ArcType>
		GraphBase;
		using typename GraphBase::NodeIdType;
		using typename GraphBase::NodeMap;

		using typename GraphBase::ArcIdType;
		using typename GraphBase::ArcMap;

		public:
			MOCK_METHOD(void, insert, (NodeType*), (override));
			MOCK_METHOD(void, insert, (ArcType*), (override));

			MOCK_METHOD(void, erase, (NodeType*), (override));
			MOCK_METHOD(void, erase, (ArcType*), (override));

			MOCK_METHOD(void, addCallOnInsertNode, (FPMAS::api::utils::Callback<NodeType*>*), (override));
			MOCK_METHOD(void, addCallOnEraseNode, (FPMAS::api::utils::Callback<NodeType*>*), (override));
			MOCK_METHOD(void, addCallOnInsertArc, (FPMAS::api::utils::Callback<ArcType*>*), (override));
			MOCK_METHOD(void, addCallOnEraseArc, (FPMAS::api::utils::Callback<ArcType*>*), (override));

			MOCK_METHOD(const NodeIdType&, currentNodeId, (), (const, override));
			MOCK_METHOD(NodeType*, getNode, (NodeIdType), (override));
			MOCK_METHOD(const NodeType*, getNode, (NodeIdType), (const, override));
			MOCK_METHOD(const NodeMap&, getNodes, (), (const, override));

			MOCK_METHOD(const ArcIdType&, currentArcId, (), (const, override));
			MOCK_METHOD(ArcType*, getArc, (ArcIdType), (override));
			MOCK_METHOD(const ArcType*, getArc, (ArcIdType), (const, override));
			MOCK_METHOD(const ArcMap&, getArcs, (), (const, override));

			MOCK_METHOD(void, removeNode, (NodeType*), (override));
			MOCK_METHOD(void, unlink, (ArcType*), (override));

			MOCK_METHOD(void, clear, (), (override));
};
#endif
