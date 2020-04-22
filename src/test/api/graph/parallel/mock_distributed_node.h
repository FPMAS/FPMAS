#ifndef MOCK_DISTRIBUTED_NODE_H
#define MOCK_DISTRIBUTED_NODE_H

#include "gmock/gmock.h"

#include "api/graph/base/mock_node.h"
#include "api/graph/parallel/distributed_node.h"
#include "mock_distributed_arc.h"

using ::testing::_;

using FPMAS::api::graph::parallel::LocationState;

template<typename T, template<typename> class Mutex>
class MockDistributedNode;
template<typename T, template<typename> class Mutex>
void from_json(const nlohmann::json& j, MockDistributedNode<T, Mutex>& mock);

template<typename T, template<typename> class Mutex>
class MockDistributedNode :
	public FPMAS::api::graph::parallel::DistributedNode<T, MockDistributedArc<T, Mutex>>,
	public AbstractMockNode<T, DistributedId, MockDistributedArc<T, Mutex>> {

		typedef AbstractMockNode<T, DistributedId, MockDistributedArc<T, Mutex>>
			node_base;
		friend void from_json<T>(const nlohmann::json&, MockDistributedNode<T, Mutex>&);

		private:
		LocationState _state = LocationState::LOCAL;
		int _location;

		public:
		using typename node_base::arc_type;
		MockDistributedNode() {
			setUpLocationAccess();
			setUpStateAccess();
		}
		MockDistributedNode(const MockDistributedNode& otherMock):
			MockDistributedNode(
					otherMock.getId(), otherMock.data(),
					otherMock.getWeight()
					){
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}
		MockDistributedNode<int, Mutex>& operator=(const MockDistributedNode<int, Mutex>& other) {
			setUpLocationAccess();
			setUpStateAccess();
			return *this;
		}

		MockDistributedNode(const DistributedId& id)
			: node_base(id) {
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MockDistributedNode(const DistributedId& id, const T& data)
			: node_base(id, std::move(data)) {
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}
		MockDistributedNode(const DistributedId& id, const T& data, float weight)
			: node_base(id, std::move(data), weight) {
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MOCK_METHOD(int, getLocation, (), (const, override));
		MOCK_METHOD(void, setLocation, (int), (override));

		MOCK_METHOD(void, setState, (LocationState), (override));
		MOCK_METHOD(LocationState, state, (), (const, override));

		MOCK_METHOD(
				FPMAS::api::graph::parallel::synchro::Mutex<T>&, mutex, (), (override)
				);

		private:
		void setUpStateAccess() {
			ON_CALL(*this, setState)
				.WillByDefault(SaveArg<0>(&_state));
			ON_CALL(*this, state)
				.WillByDefault(ReturnPointee(&_state));
		}
		void setUpLocationAccess() {
			ON_CALL(*this, setLocation)
				.WillByDefault(SaveArg<0>(&_location));
			ON_CALL(*this, getLocation)
				.WillByDefault(ReturnPointee(&_location));
		}
		void anyExpectations() {
			EXPECT_CALL(*this, linkIn).Times(AnyNumber());
			EXPECT_CALL(*this, linkOut).Times(AnyNumber());
			EXPECT_CALL(*this, data()).Times(AnyNumber());
			EXPECT_CALL(Const(*this), data()).Times(AnyNumber());
			EXPECT_CALL(*this, getWeight).Times(AnyNumber());
			EXPECT_CALL(*this, setState).Times(AnyNumber());
			EXPECT_CALL(*this, state).Times(AnyNumber());
			EXPECT_CALL(*this, getIncomingArcs()).Times(AnyNumber());
			EXPECT_CALL(*this, getIncomingArcs(_)).Times(AnyNumber());
			EXPECT_CALL(*this, getOutgoingArcs()).Times(AnyNumber());
			EXPECT_CALL(*this, getOutgoingArcs(_)).Times(AnyNumber());
			EXPECT_CALL(*this, setLocation).Times(AnyNumber());
			EXPECT_CALL(*this, getLocation).Times(AnyNumber());
		}

	};

template<typename T, template<typename> class Mutex>
void to_json(nlohmann::json& j, const MockDistributedNode<T, Mutex>& mock) {
	j["id"] = mock.getId();
	j["data"] = mock.data();
	j["weight"] = mock.getWeight();
};

template<typename T, template<typename> class Mutex>
void from_json(const nlohmann::json& j, MockDistributedNode<T, Mutex>& mock) {
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
