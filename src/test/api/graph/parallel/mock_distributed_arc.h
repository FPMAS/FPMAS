#ifndef MOCK_DISTRIBUTED_ARC_H
#define MOCK_DISTRIBUTED_ARC_H

#include "gmock/gmock.h"

#include "api/graph/base/mock_arc.h"
#include "api/graph/parallel/distributed_arc.h"

using FPMAS::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedArc;
template<typename T>
void from_json(const nlohmann::json& j, MockDistributedArc<T>& mock);

template<typename>
class MockDistributedNode;

template<typename T>
class MockDistributedArc :
	public FPMAS::api::graph::parallel::DistributedArc<T, MockDistributedNode<T>>,
	public AbstractMockArc<T, DistributedId, MockDistributedNode<T>>
{
	typedef AbstractMockArc<T, DistributedId, MockDistributedNode<T>> arc_base;
	typedef FPMAS::api::graph::parallel::DistributedArc<T, MockDistributedNode<T>> dist_arc_base;

	friend void from_json<T>(const nlohmann::json&, MockDistributedArc<T>&);

	public:
	using typename arc_base::node_type;
	using typename dist_arc_base::layer_id_type;

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

	// Used internally by nlohmann json
	MockDistributedArc<int>& operator=(const MockDistributedArc<int>& other) {
		// Copy saved fields
		this->id = other.id;
		this->src = other.src;
		this->tgt = other.tgt;
		this->_state = other._state;
		return *this;
	}

	MockDistributedArc(
			const DistributedId& id, layer_id_type layer)
		: arc_base(id, layer) {
			setUpStateAccess();
			this->anyExpectations();
		}
	MockDistributedArc(
			const DistributedId& id,
			layer_id_type layer, float weight)
		: arc_base(id, layer, weight) {
			setUpStateAccess();
			this->anyExpectations();
		}

	MOCK_METHOD(void, setState, (LocationState), (override));
	MOCK_METHOD(LocationState, state, (), (const, override));

	private:
	void setUpStateAccess() {
		EXPECT_CALL(*this, setState)
			.Times(AnyNumber())
			.WillRepeatedly(SaveArg<0>(&_state));
		ON_CALL(*this, state)
			.WillByDefault(ReturnPointee(&_state));
	}
	void anyExpectations() {
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

	mock.setSourceNode(new MockDistributedNode<T>(
						j.at("src").get<DistributedId>()
						));
	mock.setTargetNode(new MockDistributedNode<T>(
						j.at("tgt").get<DistributedId>()
						));

	ON_CALL(mock, getLayer)
		.WillByDefault(Return(
			j.at("layer").get<typename MockDistributedArc<T>::layer_id_type>()
		));
	ON_CALL(mock, getWeight)
		.WillByDefault(Return(j.at("weight").get<float>()));
	mock.anyExpectations();

}
#endif
