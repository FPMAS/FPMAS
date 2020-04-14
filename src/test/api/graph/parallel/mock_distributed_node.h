#ifndef MOCK_DISTRIBUTED_NODE_H
#define MOCK_DISTRIBUTED_NODE_H

#include "gmock/gmock.h"

#include "api/graph/base/mock_node.h"
#include "api/graph/parallel/distributed_node.h"

using FPMAS::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedNode :
	public FPMAS::api::graph::parallel::DistributedNode<T>,
	public AbstractMockNode<T, DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>> {

	typedef AbstractMockNode<T, DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>>
		node_base;

	public:
		MockDistributedNode() {}
		MockDistributedNode(const DistributedId& id, LocationState state)
			: node_base(id) {
				ON_CALL(*this, state)
					.WillByDefault(Return(state));
			}
		MockDistributedNode(const DistributedId& id, T&& data, LocationState state)
			: node_base(id, std::move(data)) {
				ON_CALL(*this, state)
					.WillByDefault(Return(state));
			}
		MockDistributedNode(const DistributedId& id, T&& data, float weight, LocationState state)
			: node_base(id, std::move(data), weight) {
				ON_CALL(*this, state)
					.WillByDefault(Return(state));
			}

		MOCK_METHOD(int, getLocation, (), (const, override));
		MOCK_METHOD(void, setLocation, (int), (override));

		MOCK_METHOD(LocationState, state, (), (const, override));
};
#endif
