#ifndef MOCK_DISTRIBUTED_ARC_H
#define MOCK_DISTRIBUTED_ARC_H

#include "gmock/gmock.h"

#include "../base/mock_arc.h"
#include "fpmas/api/graph/parallel/distributed_arc.h"
#include "mock_distributed_node.h"

using fpmas::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedArc;
template<typename T>
void from_json(const nlohmann::json& j, MockDistributedArc<T>& mock);

template<typename>
class MockDistributedNode;

template<typename T>
class MockDistributedArc :
	public fpmas::api::graph::parallel::DistributedArc<T>,
	public AbstractMockArc<DistributedId, fpmas::api::graph::parallel::DistributedNode<T>>
{
	typedef AbstractMockArc<DistributedId, fpmas::api::graph::parallel::DistributedNode<T>> ArcBase;
	typedef fpmas::api::graph::parallel::DistributedArc<T> DistArcBase;

	friend void from_json<T>(const nlohmann::json&, MockDistributedArc<T>&);

	public:
	using typename ArcBase::NodeType;
	using typename DistArcBase::LayerIdType;

	// Saved LocationState
	LocationState _state = LocationState::LOCAL;

	MockDistributedArc() {
	}

	MockDistributedArc(const MockDistributedArc& otherMock) :
		MockDistributedArc(
				otherMock.getId(),
				otherMock.getLayer(),
				otherMock.getWeight()
				) {
			this->src = otherMock.src;
			this->tgt = otherMock.tgt;
		}

	MockDistributedArc(
			const DistributedId& id, LayerIdType layer)
		: ArcBase(id, layer) {
			setUpGetters();
		}
	MockDistributedArc(
			const DistributedId& id,
			LayerIdType layer, float weight)
		: ArcBase(id, layer, weight) {
			setUpGetters();
		}

	MOCK_METHOD(void, setState, (LocationState), (override));
	MOCK_METHOD(LocationState, state, (), (const, override));

	bool operator==(const MockDistributedArc& other) const {
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
inline void to_json(nlohmann::json& j, const MockDistributedArc<T>& mock) {
	j["id"] = mock.getId();
	j["src"] = mock.getSourceNode()->getId();
	j["tgt"] = mock.getTargetNode()->getId();
	j["layer"] = mock.getLayer();
	j["weight"] = mock.getWeight();
}

template<typename T>
inline void from_json(const nlohmann::json& j, MockDistributedArc<T>& mock) {
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
			j.at("layer").get<typename MockDistributedArc<T>::LayerIdType>()
		));
	ON_CALL(mock, getWeight)
		.WillByDefault(Return(j.at("weight").get<float>()));
	mock.anyExpectations();

}
#endif
