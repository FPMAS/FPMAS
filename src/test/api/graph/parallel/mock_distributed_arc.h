#ifndef MOCK_DISTRIBUTED_ARC_H
#define MOCK_DISTRIBUTED_ARC_H

#include "gmock/gmock.h"

#include "api/graph/base/mock_arc.h"
#include "api/graph/parallel/distributed_arc.h"
#include "api/graph/parallel/mock_distributed_node.h"

using FPMAS::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedArc;
template<typename T>
void from_json(const nlohmann::json& j, MockDistributedArc<T>& mock);

template<typename T>
class MockDistributedArc :
	public FPMAS::api::graph::parallel::DistributedArc<T>,
	public AbstractMockArc<T, DistributedId, FPMAS::api::graph::parallel::DistributedNode<T>>
{
	typedef AbstractMockArc<T, DistributedId, FPMAS::api::graph::parallel::DistributedNode<T>> arc_base;
	typedef FPMAS::api::graph::parallel::DistributedArc<T> dist_arc_base;

	friend void from_json<T>(const nlohmann::json&, MockDistributedArc<T>&);

	public:
	using typename arc_base::node_type;
	using typename dist_arc_base::layer_id_type;

	MockDistributedArc() {}
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
	MockDistributedArc<int>& operator=(const MockDistributedArc<int>& other) {
		return *this;
	}

	MockDistributedArc(
			const DistributedId& id, node_type* src, node_type* tgt,
			layer_id_type layer, LocationState state)
		: arc_base(id, src, tgt, layer) {
			ON_CALL(*this, state)
				.WillByDefault(Return(state));
			this->anyExpectations();
		}
	MockDistributedArc(
			const DistributedId& id, node_type* src, node_type* tgt,
			layer_id_type layer, float weight, LocationState state)
		: arc_base(id, src, tgt, layer, weight) {
			ON_CALL(*this, state)
				.WillByDefault(Return(state));
			this->anyExpectations();
		}

	MOCK_METHOD(void, setState, (LocationState), (override));
	MOCK_METHOD(LocationState, state, (), (const, override));

	private:
	void anyExpectations() {
		EXPECT_CALL(*this, getSourceNode()).Times(AnyNumber());
		EXPECT_CALL(*this, getTargetNode()).Times(AnyNumber());
		EXPECT_CALL(*this, getWeight()).Times(AnyNumber());
		EXPECT_CALL(*this, getLayer()).Times(AnyNumber());
		EXPECT_CALL(*this, state()).Times(AnyNumber());
	}
};

template<typename T>
void to_json(nlohmann::json& j, const MockDistributedArc<T>& mock) {
	j["id"] = mock.getId();
	j["src"] = mock.getSourceNode()->getId();
	j["tgt"] = mock.getTargetNode()->getId();
	j["layer"] = mock.getLayer();
	j["weight"] = mock.getWeight();
};

template<typename T>
void from_json(const nlohmann::json& j, MockDistributedArc<T>& mock) {
	ON_CALL(mock, getId)
		.WillByDefault(Return(j.at("id").get<DistributedId>()));
	EXPECT_CALL(mock, getId).Times(AnyNumber());
	ON_CALL(mock, getSourceNode)
		.WillByDefault(Return(new MockDistributedNode<T>(
						j.at("src").get<DistributedId>()
						)));
	ON_CALL(mock, getTargetNode)
		.WillByDefault(Return(new MockDistributedNode<T>(
						j.at("tgt").get<DistributedId>()
						)));
	ON_CALL(mock, getLayer)
		.WillByDefault(Return(
			j.at("layer").get<typename MockDistributedArc<T>::layer_id_type>()
		));
	ON_CALL(mock, getWeight)
		.WillByDefault(Return(j.at("weight").get<float>()));
	mock.anyExpectations();

}

#endif
