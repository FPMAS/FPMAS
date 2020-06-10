#ifndef MOCK_DISTRIBUTED_GRAPH_H
#define MOCK_DISTRIBUTED_GRAPH_H
#include "gmock/gmock.h"

#include "../base/mock_graph_base.h"
#include "api/graph/parallel/distributed_graph.h"

template<typename T, typename DistNode, typename DistArc>
class MockDistributedGraph :
	public MockGraph<
		FPMAS::api::graph::parallel::DistributedNode<T>,
		FPMAS::api::graph::parallel::DistributedArc<T>>,
	public FPMAS::api::graph::parallel::DistributedGraph<T> {

		typedef FPMAS::api::graph::parallel::DistributedGraph<T>
			GraphBase;
		public:
		typedef FPMAS::api::graph::parallel::DistributedNode<T> NodeType;
		typedef FPMAS::api::graph::parallel::DistributedArc<T> ArcType;
		using typename GraphBase::NodeMap;
		using typename GraphBase::ArcMap;
		using typename GraphBase::PartitionMap;
		using typename GraphBase::LayerIdType;

		MOCK_METHOD(
				const FPMAS::api::graph::parallel::LocationManager<T>&,
				getLocationManager, (), (const, override));

		MOCK_METHOD(NodeType*, buildNode, (const T&), (override));
		MOCK_METHOD(NodeType*, buildNode, (T&&), (override));
		MOCK_METHOD(ArcType*, link, (NodeType*, NodeType*, LayerIdType), (override));

		MOCK_METHOD(NodeType*, importNode, (NodeType*), (override));
		MOCK_METHOD(ArcType*, importArc, (ArcType*), (override));
		MOCK_METHOD(void, clear, (ArcType*), (override));
		MOCK_METHOD(void, clear, (NodeType*), (override));

		MOCK_METHOD(void, balance, (FPMAS::api::load_balancing::LoadBalancing<T>&), (override));
		MOCK_METHOD(void, balance, (FPMAS::api::load_balancing::FixedVerticesLoadBalancing<T>&, PartitionMap), (override));
		MOCK_METHOD(void, distribute, (PartitionMap), (override));
		MOCK_METHOD(void, synchronize, (), (override));
	};

#endif
