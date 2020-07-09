#ifndef MOCK_DISTRIBUTED_EDGE_H
#define MOCK_DISTRIBUTED_EDGE_H

#include "gmock/gmock.h"

#include "../base/mock_edge.h"
#include "fpmas/api/graph/parallel/distributed_edge.h"
#include "mock_distributed_node.h"

using fpmas::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedEdge;
template<typename T>
void from_json(const nlohmann::json& j, MockDistributedEdge<T>& mock);

template<typename>
class MockDistributedNode;

template<typename T>
class MockDistributedEdge :
	public fpmas::api::graph::parallel::DistributedEdge<T>,
	public AbstractMockEdge<DistributedId, fpmas::api::graph::parallel::DistributedNode<T>>
{
	typedef AbstractMockEdge<DistributedId, fpmas::api::graph::parallel::DistributedNode<T>> EdgeBase;
	typedef fpmas::api::graph::parallel::DistributedEdge<T> DistEdgeBase;

	friend void from_json<T>(const nlohmann::json&, MockDistributedEdge<T>&);

	public:
	using typename EdgeBase::NodeType;
	using typename DistEdgeBase::LayerIdType;

	// Saved LocationState
	LocationState _state = LocationState::LOCAL;

	MockDistributedEdge() {
	}

	MockDistributedEdge(const MockDistributedEdge& otherMock) :
		MockDistributedEdge(
				otherMock.getId(),
				otherMock.getLayer(),
				otherMock.getWeight()
				) {
			this->src = otherMock.src;
			this->tgt = otherMock.tgt;
		}

	MockDistributedEdge(
			const DistributedId& id, LayerIdType layer)
		: EdgeBase(id, layer) {
			setUpGetters();
		}
	MockDistributedEdge(
			const DistributedId& id,
			LayerIdType layer, float weight)
		: EdgeBase(id, layer, weight) {
			setUpGetters();
		}

	MOCK_METHOD(void, setState, (LocationState), (override));
	MOCK_METHOD(LocationState, state, (), (const, override));

	bool operator==(const MockDistributedEdge& other) const {
		return this->id == other.id;
	}

	private:
	void setUpGetters() {
		EXPECT_CALL(*this, setState)
			.Times(AnyNumber())
			.WillRepeatedly(SaveArg<0>(&_state));
		ON_CALL(*this, state)
			.WillByDefault(ReturnPointee(&_state));
		EXPECT_CALL(*this, state()).Times(AnyNumber());
	}
};

template<typename T>
inline void to_json(nlohmann::json& j, const MockDistributedEdge<T>& mock) {
	j["id"] = mock.getId();
	j["src"] = mock.getSourceNode()->getId();
	j["tgt"] = mock.getTargetNode()->getId();
	j["layer"] = mock.getLayer();
	j["weight"] = mock.getWeight();
}

template<typename T>
inline void from_json(const nlohmann::json& j, MockDistributedEdge<T>& mock) {
	ON_CALL(mock, getId)
		.WillByDefault(Return(j.at("id").get<DistributedId>()));
	EXPECT_CALL(mock, getId).Times(AnyNumber());

	mock.setSourceNode(new MockDistributedNode<T>(
						j.at("src").get<DistributedId>()
						));
	mock.setTargetNode(new MockDistributedNode<T>(
						j.at("tgt").get<DistributedId>()
						));

	ON_CALL(mock, getLayer)
		.WillByDefault(Return(
			j.at("layer").get<typename MockDistributedEdge<T>::LayerIdType>()
		));
	ON_CALL(mock, getWeight)
		.WillByDefault(Return(j.at("weight").get<float>()));
	mock.anyExpectations();

}
#endif
