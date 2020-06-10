#ifndef MOCK_DISTRIBUTED_NODE_H
#define MOCK_DISTRIBUTED_NODE_H

#include "gmock/gmock.h"

#include "../base/mock_node.h"
#include "api/graph/parallel/distributed_node.h"
#include "mock_distributed_arc.h"

using ::testing::_;

using FPMAS::api::graph::parallel::LocationState;

template<typename T>
class MockDistributedNode;
template<typename T>
void from_json(const nlohmann::json& j, MockDistributedNode<T>& mock);

template<typename> class MockDistributedArc;

ACTION_P(ReturnPointeePointee, ptr) {return **ptr;}

template<typename T>
class MockDistributedNode :
	public FPMAS::api::graph::parallel::DistributedNode<T>,
	public AbstractMockNode<DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>> {

		typedef AbstractMockNode<DistributedId, FPMAS::api::graph::parallel::DistributedArc<T>>
			NodeBase;
		typedef FPMAS::api::graph::parallel::synchro::Mutex<T> Mutex;
		friend void from_json<T>(const nlohmann::json&, MockDistributedNode<T>&);

		private:
		T _data;
		LocationState _state = LocationState::LOCAL;
		int _location;
		FPMAS::api::graph::parallel::synchro::Mutex<T>* null_mutex = nullptr;
		Mutex** _mutex = &null_mutex;

		public:
		typedef T DataType;
		using typename NodeBase::ArcType;

		MockDistributedNode() {
			setUpDataAccess();
			setUpDefaultMutex();
			setUpLocationAccess();
			setUpStateAccess();
		}
		MockDistributedNode(const MockDistributedNode& otherMock):
			MockDistributedNode(
					otherMock.getId(), otherMock.mutex().data(),
					otherMock.getWeight()
					){
				setUpDataAccess();
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MockDistributedNode(const DistributedId& id)
			: NodeBase(id) {
				setUpDataAccess();
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MockDistributedNode(const DistributedId& id, const T& data)
			: NodeBase(id), _data(data) {
				setUpDataAccess();
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}
		MockDistributedNode(const DistributedId& id, const T& data, float weight)
			: NodeBase(id, weight), _data(data) {
				setUpDataAccess();
				setUpDefaultMutex();
				setUpLocationAccess();
				setUpStateAccess();
				this->anyExpectations();
			}

		MOCK_METHOD(int, getLocation, (), (const, override));
		MOCK_METHOD(void, setLocation, (int), (override));

		MOCK_METHOD(void, setState, (LocationState), (override));
		MOCK_METHOD(LocationState, state, (), (const, override));

		MOCK_METHOD(T&, data, (), (override));
		MOCK_METHOD(const T&, data, (), (const, override));
		MOCK_METHOD(void, setMutex, (Mutex*), (override));
		MOCK_METHOD(Mutex&, mutex, (), (override));
		MOCK_METHOD(const Mutex&, mutex, (), (const, override));

		bool operator==(const MockDistributedNode& other) const {
			return this->id == other.id;
		}

		~MockDistributedNode() {
			if(*_mutex!=nullptr) {
				delete *_mutex;
			}
		}

		private:
		void setUpDataAccess() {
			ON_CALL(*this, data())
				.WillByDefault(ReturnRef(_data));
			EXPECT_CALL(*this, data()).Times(AnyNumber());

			ON_CALL(Const(*this), data())
				.WillByDefault(ReturnRef(_data));
			EXPECT_CALL(Const(*this), data()).Times(AnyNumber());
		}
		void setUpDefaultMutex() {
			ON_CALL(*this, setMutex)
				.WillByDefault(SaveArg<0>(_mutex));
			EXPECT_CALL(*this, setMutex).Times(AnyNumber());

			ON_CALL(*this, mutex())
				.WillByDefault(ReturnPointeePointee(_mutex));
			EXPECT_CALL(*this, mutex()).Times(AnyNumber());
			ON_CALL(Const(*this), mutex())
				.WillByDefault(ReturnPointeePointee(_mutex));
			EXPECT_CALL(Const(*this), mutex()).Times(AnyNumber());
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
			EXPECT_CALL(*this, getWeight).Times(AnyNumber());
			EXPECT_CALL(*this, setState).Times(AnyNumber());
			EXPECT_CALL(*this, state).Times(AnyNumber());
			EXPECT_CALL(*this, data()).Times(AnyNumber());
			EXPECT_CALL(Const(*this), data()).Times(AnyNumber());
			EXPECT_CALL(*this, getIncomingArcs()).Times(AnyNumber());
			EXPECT_CALL(*this, getIncomingArcs(_)).Times(AnyNumber());
			EXPECT_CALL(*this, getOutgoingArcs()).Times(AnyNumber());
			EXPECT_CALL(*this, getOutgoingArcs(_)).Times(AnyNumber());
			EXPECT_CALL(*this, setLocation).Times(AnyNumber());
			EXPECT_CALL(*this, getLocation).Times(AnyNumber());
		}
	};

template<typename T>
inline void to_json(nlohmann::json& j, const MockDistributedNode<T>& mock) {
	j["id"] = mock.getId();
	j["data"] = mock.mutex().data();
	j["weight"] = mock.getWeight();
}

template<typename T>
inline void from_json(const nlohmann::json& j, MockDistributedNode<T>& mock) {
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
