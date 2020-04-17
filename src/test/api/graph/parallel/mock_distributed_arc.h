#ifndef MOCK_DISTRIBUTED_ARC_H
#define MOCK_DISTRIBUTED_ARC_H

#include "gmock/gmock.h"

#include "api/graph/base/mock_arc.h"
#include "api/graph/parallel/distributed_arc.h"

using FPMAS::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedArc :
	public FPMAS::api::graph::parallel::DistributedArc<T>,
	public AbstractMockArc<T, DistributedId, FPMAS::api::graph::parallel::DistributedNode<T>>
{
	typedef AbstractMockArc<T, DistributedId, FPMAS::api::graph::parallel::DistributedNode<T>> arc_base;
	typedef FPMAS::api::graph::parallel::DistributedArc<T> dist_arc_base;

	public:
	using typename arc_base::node_type;
	using typename dist_arc_base::layer_id_type;

	MockDistributedArc(const MockDistributedArc& otherMock) :
		MockDistributedArc(
				otherMock.getId(),
				otherMock.getSourceNode(),
				otherMock.getTargetNode(),
				otherMock.getLayer(),
				otherMock.getWeight(),
				otherMock.state()
				) {
		}

	MockDistributedArc(
			const DistributedId& id, node_type* src, node_type* tgt,
			layer_id_type layer, LocationState state)
		: arc_base(id, src, tgt, layer) {
			ON_CALL(*this, state)
				.WillByDefault(Return(state));
			this->disableExpectations();
		}
	MockDistributedArc(
			const DistributedId& id, node_type* src, node_type* tgt,
			layer_id_type layer, float weight, LocationState state)
		: arc_base(id, src, tgt, layer, weight) {
			ON_CALL(*this, state)
				.WillByDefault(Return(state));
			this->disableExpectations();
		}

	MOCK_METHOD(void, setState, (LocationState), (override));
	MOCK_METHOD(LocationState, state, (), (const, override));

	private:
	void disableExpectations() {
		EXPECT_CALL(*this, getSourceNode()).Times(AnyNumber());
		EXPECT_CALL(*this, getTargetNode()).Times(AnyNumber());
		EXPECT_CALL(*this, getWeight()).Times(AnyNumber());
		EXPECT_CALL(*this, getLayer()).Times(AnyNumber());
		EXPECT_CALL(*this, state()).Times(AnyNumber());
	}
};

#endif
