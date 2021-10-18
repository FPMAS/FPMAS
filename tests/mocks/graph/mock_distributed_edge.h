#ifndef MOCK_DISTRIBUTED_EDGE_H
#define MOCK_DISTRIBUTED_EDGE_H

#include "gmock/gmock.h"

#include "mock_edge.h"
#include "fpmas/api/graph/distributed_edge.h"
#include "mock_distributed_node.h"

using fpmas::api::graph::LocationState;
using ::testing::Invoke;

template<typename T, template<typename> class Strictness>
class MockDistributedEdge;
template<typename T, template<typename> class Strictness>
void from_json(const nlohmann::json& j, MockDistributedEdge<T, Strictness>& mock);

template<typename, template<typename> class>
class MockDistributedNode;

template<typename T>
class MockTemporaryNode : public fpmas::api::graph::TemporaryNode<T> {
	public:
		MOCK_METHOD(fpmas::api::graph::DistributedId, getId, (), (const, override));
		MOCK_METHOD(int, getLocation, (), (const, override));
		MOCK_METHOD(fpmas::api::graph::DistributedNode<T>*, build, (), (override));
		MOCK_METHOD(void, destructor, (), ());

		~MockTemporaryNode() {
			destructor();
		}
};

template<typename T>
class AbstractMockDistributedEdge : public fpmas::api::graph::DistributedEdge<T> {
	public:
	// Saved LocationState
	LocationState _state = LocationState::LOCAL;
	std::unique_ptr<fpmas::api::graph::TemporaryNode<T>> temp_src;
	std::unique_ptr<fpmas::api::graph::TemporaryNode<T>> temp_tgt;

	MOCK_METHOD(void, setState, (LocationState), (override));
	MOCK_METHOD(LocationState, state, (), (const, override));
	MOCK_METHOD(void, setTempSourceNode,
			(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>), (override));
	MOCK_METHOD(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>,
			getTempSourceNode, (), (override));
	MOCK_METHOD(void, setTempTargetNode,
			(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>), (override));
	MOCK_METHOD(std::unique_ptr<fpmas::api::graph::TemporaryNode<T>>,
			getTempTargetNode, (), (override));

	bool operator==(const AbstractMockDistributedEdge& other) const {
		return this->id == other.id;
	}
	protected:
	AbstractMockDistributedEdge() {
		this->setUpGetters();
	}

	private:
	void setUpGetters() {
		ON_CALL(*this, setState)
			.WillByDefault(SaveArg<0>(&_state));
		ON_CALL(*this, state)
			.WillByDefault(ReturnPointee(&_state));

		ON_CALL(*this, setTempSourceNode)
			.WillByDefault(Invoke([this] (
							std::unique_ptr<fpmas::api::graph::TemporaryNode<T>> temp_node) {
						this->temp_src = std::move(temp_node);
						}
						));
		ON_CALL(*this, getTempSourceNode)
			.WillByDefault(Invoke([this] ()  {
						return std::move(temp_src);
						}));

		ON_CALL(*this, setTempTargetNode)
			.WillByDefault(Invoke([this] (
							std::unique_ptr<fpmas::api::graph::TemporaryNode<T>> temp_node) {
						this->temp_tgt = std::move(temp_node);
						}
						));
		ON_CALL(*this, getTempTargetNode)
			.WillByDefault(Invoke([this] ()  {
						return std::move(temp_tgt);
						}));
	}
};

template<typename T, template<typename> class Strictness = testing::NaggyMock>
class MockDistributedEdge :
	public Strictness<AbstractMockDistributedEdge<T>>,
	public Strictness<AbstractMockEdge<DistributedId, fpmas::api::graph::DistributedNode<T>>> {
		friend void from_json<T, Strictness>(const nlohmann::json&, MockDistributedEdge<T, Strictness>&);
		typedef Strictness<AbstractMockEdge<DistributedId, fpmas::api::graph::DistributedNode<T>>>
			EdgeBase;
		typedef Strictness<AbstractMockDistributedEdge<T>>
			DistEdgeBase;

		public:
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
				const DistributedId& id, LayerId layer)
			: EdgeBase(id, layer) {
			}
		MockDistributedEdge(
				const DistributedId& id,
				LayerId layer,
				float weight)
			: EdgeBase(id, layer, weight) {
			}
	};

template<typename T, template<typename> class Strictness>
inline void to_json(nlohmann::json& j, const MockDistributedEdge<T, Strictness>& mock) {
	j["id"] = mock.getId();
	j["src"] = mock.getSourceNode()->getId();
	j["tgt"] = mock.getTargetNode()->getId();
	j["layer"] = mock.getLayer();
	j["weight"] = mock.getWeight();
}

template<typename T, template<typename> class Strictness>
inline void from_json(const nlohmann::json& j, MockDistributedEdge<T, Strictness>& mock) {
	ON_CALL(mock, getId)
		.WillByDefault(Return(j.at("id").get<DistributedId>()));
	EXPECT_CALL(mock, getId).Times(AnyNumber());

	mock.setSourceNode(new MockDistributedNode<T, testing::NiceMock>(
						j.at("src").get<DistributedId>()
						));
	mock.setTargetNode(new MockDistributedNode<T, testing::NiceMock>(
						j.at("tgt").get<DistributedId>()
						));

	ON_CALL(mock, getLayer)
		.WillByDefault(Return(
			j.at("layer").get<typename MockDistributedEdge<T, Strictness>::LayerId>()
		));
	ON_CALL(mock, getWeight)
		.WillByDefault(Return(j.at("weight").get<float>()));
	mock.anyExpectations();
}
#endif
