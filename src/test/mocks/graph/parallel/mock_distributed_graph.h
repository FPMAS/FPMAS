#ifndef MOCK_DISTRIBUTED_GRAPH_H
#define MOCK_DISTRIBUTED_GRAPH_H
#include "gmock/gmock.h"

#include "../base/mock_graph_base.h"
#include "api/graph/parallel/distributed_graph.h"

template<typename T, typename DistNode, typename DistArc>
class MockDistributedGraph :
	public MockGraphBase<
		FPMAS::api::graph::parallel::DistributedNode<T>,
		FPMAS::api::graph::parallel::DistributedArc<T>>,
	public FPMAS::api::graph::parallel::DistributedGraph<T> {

		typedef FPMAS::api::graph::parallel::DistributedGraph<T>
			GraphBase;
		public:
		using typename GraphBase::NodeType;
		using typename GraphBase::NodeMap;
		using typename GraphBase::ArcType;
		using typename GraphBase::ArcMap;
		using typename GraphBase::PartitionMap;

		MOCK_METHOD(
				const FPMAS::api::graph::parallel::LocationManager<FPMAS::api::graph::parallel::DistributedNode<T>>&,
				getLocationManager, (), (const, override));

		MOCK_METHOD(NodeType*, importNode, (const NodeType&), (override));
		MOCK_METHOD(ArcType*, importArc, (const ArcType&), (override));
		MOCK_METHOD(void, clear, (ArcType*), (override));
		MOCK_METHOD(void, clear, (NodeType*), (override));

		MOCK_METHOD(void, balance, (), (override));
		MOCK_METHOD(void, distribute, (PartitionMap), (override));
		MOCK_METHOD(void, synchronize, (), (override));
	};

#endif
