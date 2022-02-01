#ifndef MOCK_DISTRIBUTED_GRAPH_H
#define MOCK_DISTRIBUTED_GRAPH_H
#include "gmock/gmock.h"

#include "mock_graph_base.h"
#include "fpmas/api/graph/distributed_graph.h"
#include "mock_distributed_node.h"
#include "mock_distributed_edge.h"

template<typename T, typename DistNode, typename DistEdge>
class AbstractMockDistributedGraph :
	public fpmas::api::graph::DistributedGraph<T> {

		typedef fpmas::api::graph::DistributedGraph<T>
			GraphBase;
		public:
		typedef fpmas::api::graph::DistributedNode<T> NodeType;
		typedef fpmas::api::graph::DistributedEdge<T> EdgeType;
		using typename GraphBase::NodeMap;
		using typename GraphBase::EdgeMap;
		using typename GraphBase::SetLocalNodeCallback;
		using typename GraphBase::SetDistantNodeCallback;

		MOCK_METHOD(const fpmas::api::communication::MpiCommunicator&,
				getMpiCommunicator, (), (const, override));
		MOCK_METHOD(fpmas::api::communication::MpiCommunicator&,
				getMpiCommunicator, (), (override));
		MOCK_METHOD(
				const fpmas::api::graph::LocationManager<T>&,
				getLocationManager, (), (const, override));
		MOCK_METHOD(
				fpmas::api::graph::LocationManager<T>&,
				getLocationManager, (), (override));

		NodeType* buildNode(T&& data) override {
			return buildNode_rv(data);
		}
		MOCK_METHOD(NodeType*, buildNode, (const T&), (override));
		MOCK_METHOD(NodeType*, buildNode_rv, (T&), ());
		MOCK_METHOD(fpmas::api::graph::DistributedNode<T>*, insertDistant, (fpmas::api::graph::DistributedNode<T>*), (override));
		MOCK_METHOD(EdgeType*, link, (NodeType*, NodeType*, fpmas::api::graph::LayerId), (override));

		MOCK_METHOD(void, addCallOnSetLocal, (SetLocalNodeCallback*), (override));
		MOCK_METHOD(std::vector<SetLocalNodeCallback*>, onSetLocalCallbacks, (), (const, override));
		MOCK_METHOD(void, addCallOnSetDistant, (SetDistantNodeCallback*), (override));
		MOCK_METHOD(std::vector<SetDistantNodeCallback*>, onSetDistantCallbacks, (), (const, override));

		MOCK_METHOD(DistributedId, currentNodeId, (), (const, override));
		MOCK_METHOD(void, setCurrentNodeId, (fpmas::api::graph::DistributedId), (override));
		MOCK_METHOD(DistributedId, currentEdgeId, (), (const, override));
		MOCK_METHOD(void, setCurrentEdgeId, (fpmas::api::graph::DistributedId), (override));

		MOCK_METHOD(void, removeNode, (NodeType*), (override));
		MOCK_METHOD(void, removeNode, (DistributedId), (override));
		MOCK_METHOD(void, unlink, (EdgeType*), (override));
		MOCK_METHOD(void, unlink, (DistributedId), (override));
		MOCK_METHOD(void, switchLayer, (EdgeType*, LayerId), (override));

		MOCK_METHOD(NodeType*, importNode, (NodeType*), (override));
		MOCK_METHOD(EdgeType*, importEdge, (EdgeType*), (override));
		MOCK_METHOD(
				std::unordered_set<fpmas::api::graph::DistributedNode<T>*>,
				getUnsyncNodes, (), (const, override)
				);

		MOCK_METHOD(void, balance, (fpmas::api::graph::LoadBalancing<T>&), (override));
		MOCK_METHOD(void, balance,
				(fpmas::api::graph::LoadBalancing<T>&, fpmas::api::graph::PartitionMode),
				(override));
		MOCK_METHOD(void, balance, (
					fpmas::api::graph::FixedVerticesLoadBalancing<T>&,
					fpmas::api::graph::PartitionMap),
				(override));
		MOCK_METHOD(void, balance, (
					fpmas::api::graph::FixedVerticesLoadBalancing<T>&,
					fpmas::api::graph::PartitionMap,
					fpmas::api::graph::PartitionMode),
				(override));
		MOCK_METHOD(void, distribute, (fpmas::api::graph::PartitionMap), (override));
		MOCK_METHOD(void, synchronize, (), (override));
		MOCK_METHOD(void, synchronize, (
					std::unordered_set<fpmas::api::graph::DistributedNode<T>*>, bool
					), (override));
		MOCK_METHOD(fpmas::api::synchro::SyncMode<T>&, synchronizationMode, (), (override));
	};

template<
	typename T,
	typename DistNode = MockDistributedNode<T>,
	typename DistEdge = MockDistributedEdge<T>,
	template<typename> class Strictness = testing::NaggyMock>
class MockDistributedGraph :
	public Strictness<MockGraph<
		fpmas::api::graph::DistributedNode<T>,
		fpmas::api::graph::DistributedEdge<T>>>,
	public Strictness<AbstractMockDistributedGraph<T, DistNode, DistEdge>> {
	};
#endif
