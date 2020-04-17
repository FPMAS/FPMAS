#ifndef MOCK_DISTRIBUTED_NODE_H
#define MOCK_DISTRIBUTED_NODE_H

#include "gmock/gmock.h"

#include "api/graph/base/mock_node.h"
#include "api/graph/parallel/distributed_node.h"

using FPMAS::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedNode;
template<typename T>
void from_json(const nlohmann::json& j, MockDistributedNode<T>& mock);

template<typename T>
class MockDistributedNode :
	public FPMAS::api::graph::parallel::DistributedNode<T>,
	public AbstractMockNode<T, DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>> {

	typedef AbstractMockNode<T, DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>>
		node_base;
	friend void from_json<T>(const nlohmann::json&, MockDistributedNode<T>&);

	public:
		using typename node_base::arc_type;
		MockDistributedNode() {}
		MockDistributedNode(const MockDistributedNode& otherMock):
			MockDistributedNode(
					otherMock.getId(), otherMock.data(),
					otherMock.getWeight(), otherMock.state()
					){
				this->anyExpectations();
			}
		MockDistributedNode<int>& operator=(const MockDistributedNode<int>& other) {
			return *this;
		}

		MockDistributedNode(const DistributedId& id)
			: node_base(id) {
				this->anyExpectations();
			}
		MockDistributedNode(const DistributedId& id, LocationState state)
			: node_base(id) {
				ON_CALL(*this, state)
					.WillByDefault(Return(state));
				this->anyExpectations();
			}
		MockDistributedNode(const DistributedId& id, const T& data, LocationState state)
			: node_base(id, std::move(data)) {
				ON_CALL(*this, state)
					.WillByDefault(Return(state));
				this->anyExpectations();
			}
		MockDistributedNode(const DistributedId& id, const T& data, float weight, LocationState state)
			: node_base(id, std::move(data), weight) {
				ON_CALL(*this, state)
					.WillByDefault(Return(state));
				this->anyExpectations();
			}

		MOCK_METHOD(int, getLocation, (), (const, override));
		MOCK_METHOD(void, setLocation, (int), (override));

		MOCK_METHOD(void, setState, (LocationState), (override));
		MOCK_METHOD(LocationState, state, (), (const, override));

		void anyExpectations() {
			EXPECT_CALL(*this, data()).Times(AnyNumber());
			EXPECT_CALL(Const(*this), data()).Times(AnyNumber());
			EXPECT_CALL(*this, getWeight()).Times(AnyNumber());
			EXPECT_CALL(*this, state()).Times(AnyNumber());
			EXPECT_CALL(*this, getIncomingArcs()).Times(AnyNumber());
			EXPECT_CALL(*this, getOutgoingArcs()).Times(AnyNumber());
		}

};

template<typename T>
void to_json(nlohmann::json& j, const MockDistributedNode<T>& mock) {
	j["id"] = mock.getId();
	j["data"] = mock.data();
	j["weight"] = mock.getWeight();
};

template<typename T>
void from_json(const nlohmann::json& j, MockDistributedNode<T>& mock) {
	ON_CALL(mock, getId)
		.WillByDefault(Return(j.at("id").get<DistributedId>()));
	EXPECT_CALL(mock, getId).Times(AnyNumber());
	mock._data = j.at("data").get<T>();
	ON_CALL(mock, data())
		.WillByDefault(ReturnRef(mock._data));
	ON_CALL(Const(mock), data())
		.WillByDefault(ReturnRef(mock._data));
	ON_CALL(mock, getWeight)
		.WillByDefault(Return(j.at("weight").get<float>()));
	mock.anyExpectations();
}

#endif
