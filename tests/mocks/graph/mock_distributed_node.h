#ifndef MOCK_DISTRIBUTED_NODE_H
#define MOCK_DISTRIBUTED_NODE_H

#include "gmock/gmock.h"

#include "mock_node.h"
#include "fpmas/api/graph/distributed_node.h"
#include "mock_distributed_edge.h"

using ::testing::_;

using fpmas::api::graph::LocationState;

template<typename T, template<typename> class Strictness>
class MockDistributedNode;
template<typename T, template<typename> class Strictness>
void from_json(const nlohmann::json& j, MockDistributedNode<T, Strictness>& mock);
template<typename T, template<typename> class Strictness>
void to_json(nlohmann::json& j, const MockDistributedNode<T, Strictness>& mock);

template<typename> class MockDistributedEdge;

template<typename T>
class AbstractMockDistributedNode : public fpmas::api::graph::DistributedNode<T> {
	typedef fpmas::api::synchro::Mutex<T> Mutex;

	private:
	LocationState _state = LocationState::LOCAL;
	int _location;
	fpmas::api::synchro::Mutex<T>* null_mutex = nullptr;
	Mutex** _mutex = &null_mutex;
	protected:
	T _data;

	AbstractMockDistributedNode() {
		setUpDataAccess();
		setUpDefaultMutex();
		setUpLocationAccess();
		setUpStateAccess();
		this->anyExpectations();
	}

	AbstractMockDistributedNode(const T& data)
		: _data(data) {
			setUpDataAccess();
			setUpDefaultMutex();
			setUpLocationAccess();
			setUpStateAccess();
			this->anyExpectations();
		}
	AbstractMockDistributedNode(T&& data)
		: _data(std::move(data)) {
			setUpDataAccess();
			setUpDefaultMutex();
			setUpLocationAccess();
			setUpStateAccess();
			this->anyExpectations();
		}

	public:
	typedef T DataType;

	MOCK_METHOD(int, location, (), (const, override));
	MOCK_METHOD(void, setLocation, (int), (override));

	MOCK_METHOD(void, setState, (LocationState), (override));
	MOCK_METHOD(LocationState, state, (), (const, override));

	MOCK_METHOD(T&, data, (), (override));
	MOCK_METHOD(const T&, data, (), (const, override));
	MOCK_METHOD(void, setMutex, (Mutex*), (override));
	MOCK_METHOD(Mutex*, mutex, (), (override));
	MOCK_METHOD(const Mutex*, mutex, (), (const, override));

	bool operator==(const AbstractMockDistributedNode& other) const {
		return this->id == other.id;
	}

	~AbstractMockDistributedNode() {
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
			.WillByDefault(ReturnPointee(_mutex));
		EXPECT_CALL(*this, mutex()).Times(AnyNumber());
		ON_CALL(Const(*this), mutex())
			.WillByDefault(ReturnPointee(_mutex));
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
		ON_CALL(*this, location)
			.WillByDefault(ReturnPointee(&_location));
	}
	void anyExpectations() {
		/*
		 *EXPECT_CALL(*this, linkIn).Times(AnyNumber());
		 *EXPECT_CALL(*this, linkOut).Times(AnyNumber());
		 *EXPECT_CALL(*this, getWeight).Times(AnyNumber());
		 *EXPECT_CALL(*this, setState).Times(AnyNumber());
		 *EXPECT_CALL(*this, state).Times(AnyNumber());
		 *EXPECT_CALL(*this, data()).Times(AnyNumber());
		 *EXPECT_CALL(Const(*this), data()).Times(AnyNumber());
		 *EXPECT_CALL(*this, getIncomingEdges()).Times(AnyNumber());
		 *EXPECT_CALL(*this, getIncomingEdges(_)).Times(AnyNumber());
		 *EXPECT_CALL(*this, getOutgoingEdges()).Times(AnyNumber());
		 *EXPECT_CALL(*this, getOutgoingEdges(_)).Times(AnyNumber());
		 *EXPECT_CALL(*this, setLocation).Times(AnyNumber());
		 *EXPECT_CALL(*this, location).Times(AnyNumber());
		 */
	}
};

template<typename T, template<typename> class Strictness = testing::NaggyMock>
class MockDistributedNode :
	public Strictness<AbstractMockDistributedNode<T>>,
	public Strictness<AbstractMockNode<DistributedId, fpmas::api::graph::DistributedEdge<T>>> {
		friend void from_json<T>(const nlohmann::json&, MockDistributedNode<T, Strictness>&);
		friend void to_json<T>(nlohmann::json&, const MockDistributedNode<T, Strictness>&);
		typedef Strictness<AbstractMockNode<DistributedId, fpmas::api::graph::DistributedEdge<T>>>
			NodeBase;
		typedef Strictness<AbstractMockDistributedNode<T>>
			DistNodeBase;
		public:
		using typename NodeBase::EdgeType;

		MockDistributedNode()
			: DistNodeBase() {}

		MockDistributedNode(const MockDistributedNode& otherMock):
			DistNodeBase(
					otherMock.getId(), std::move(otherMock.mutex().data()),
					otherMock.getWeight()
					){
			}

		MockDistributedNode(const DistributedId& id)
			: NodeBase(id) {
			}

		MockDistributedNode(const DistributedId& id, const T& data)
			: DistNodeBase(data), NodeBase(id) {
			}

		MockDistributedNode(const DistributedId& id, T&& data)
			: DistNodeBase(std::move(data)), NodeBase(id) {
			}

		MockDistributedNode(const DistributedId& id, const T& data, float weight)
			: DistNodeBase(data), NodeBase(id, weight) {
			}

		MockDistributedNode(const DistributedId& id, T&& data, float weight)
			: DistNodeBase(std::move(data)), NodeBase(id, weight) {
			}

	};

template<typename T, template<typename> class Strictness>
inline void to_json(nlohmann::json& j, const MockDistributedNode<T, Strictness>& mock) {
	j["id"] = mock.getId();
	j["data"] = mock._data;
	j["weight"] = mock.getWeight();
}

template<typename T, template<typename> class Strictness>
inline void from_json(const nlohmann::json& j, MockDistributedNode<T, Strictness>& mock) {
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
