#ifndef MOCK_DISTRIBUTED_NODE_H
#define MOCK_DISTRIBUTED_NODE_H

#include "gmock/gmock.h"

#include "../base/mock_node.h"
#include "api/graph/parallel/distributed_node.h"
#include "mock_distributed_arc.h"

using ::testing::_;

using FPMAS::api::graph::parallel::LocationState;

template<typename T, template<typename> class Mutex>
class MockDistributedNode;
template<typename T, template<typename> class Mutex>
void from_json(const nlohmann::json& j, MockDistributedNode<T, Mutex>& mock);

template<typename, template<typename> class> class MockDistributedArc;

template<typename T, template<typename> class Mutex>
class MockDistributedNode :
	public FPMAS::api::graph::parallel::DistributedNode<T>,
	public AbstractMockNode<T, DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>> {

		typedef AbstractMockNode<T, DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>>
			NodeBase;
		friend void from_json<T>(const nlohmann::json&, MockDistributedNode<T, Mutex>&);

		private:
		Mutex<T> _mutex;
		LocationState _state = LocationState::LOCAL;
		int _location;

		public:
		typedef T DataType;
		using typename NodeBase::ArcType;
		MockDistributedNode() {
			setUpDefaultMutex();
			setUpLocationAccess();
			setUpStateAccess();
		}
		MockDistributedNode(const MockDistributedNode& otherMock):
			MockDistributedNode(
					otherMock.getId(), otherMock.data(),
					otherMock.getWeight()
					){
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MockDistributedNode(const DistributedId& id)
			: NodeBase(id) {
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MockDistributedNode(const DistributedId& id, const T& data)
			: NodeBase(id, std::move(data)) {
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}
		MockDistributedNode(const DistributedId& id, const T& data, float weight)
			: NodeBase(id, std::move(data), weight) {
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MOCK_METHOD(int, getLocation, (), (const, override));
		MOCK_METHOD(void, setLocation, (int), (override));

		MOCK_METHOD(void, setState, (LocationState), (override));
		MOCK_METHOD(LocationState, state, (), (const, override));

		MOCK_METHOD(Mutex<T>&, mutex, (), (override));

		bool operator==(const MockDistributedNode& other) const {
			return this->id == other.id;
		}

		private:
		void setUpDefaultMutex() {
			ON_CALL(*this, mutex)
				.WillByDefault(ReturnRef(_mutex));
			EXPECT_CALL(*this, mutex).Times(AnyNumber());
		}

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
inline void to_json(nlohmann::json& j, const MockDistributedNode<T, Mutex>& mock) {
	j["id"] = mock.getId();
	j["data"] = mock.data();
	j["weight"] = mock.getWeight();
}

template<typename T, template<typename> class Mutex>
inline void from_json(const nlohmann::json& j, MockDistributedNode<T, Mutex>& mock) {
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
