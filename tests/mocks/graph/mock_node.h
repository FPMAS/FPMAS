#ifndef MOCK_NODE_H
#define MOCK_NODE_H

#include "gmock/gmock.h"

#include "fpmas/api/graph/id.h"
#include "fpmas/api/graph/node.h"
#include "mock_edge.h"

using ::testing::Const;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::AnyNumber;

using fpmas::api::graph::Id;
using fpmas::api::graph::LayerId;

class MockData {};

template<typename IdType, typename _EdgeType>
class AbstractMockNode : public virtual fpmas::api::graph::Node<
				 IdType, _EdgeType> {
	protected:
		IdType id;
		float weight = 1.;

	public:
		using typename fpmas::api::graph::Node<
			IdType, _EdgeType>::EdgeType;

		AbstractMockNode() {
			// Mock initialized without id
		}
		AbstractMockNode(const IdType& id) : id(id) {
			setUpGetters();
		}
		AbstractMockNode(const IdType& id, float weight) : id(id), weight(weight) {
			setUpGetters();
		}

		MOCK_METHOD(IdType, getId, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const, override));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(const std::vector<EdgeType*>, getIncomingEdges, (), (const, override));
		MOCK_METHOD(const std::vector<EdgeType*>, getIncomingEdges, (LayerId), (const, override));
		MOCK_METHOD(const std::vector<typename EdgeType::NodeType*>, inNeighbors, (), (const, override));
		MOCK_METHOD(const std::vector<typename EdgeType::NodeType*>, inNeighbors, (LayerId), (const, override));

		MOCK_METHOD(const std::vector<EdgeType*>, getOutgoingEdges, (), (const, override));
		MOCK_METHOD(const std::vector<EdgeType*>, getOutgoingEdges, (LayerId), (const, override));
		MOCK_METHOD(const std::vector<typename EdgeType::NodeType*>, outNeighbors, (), (const, override));
		MOCK_METHOD(const std::vector<typename EdgeType::NodeType*>, outNeighbors, (LayerId), (const, override));

		MOCK_METHOD(void, linkIn, (EdgeType*), (override));
		MOCK_METHOD(void, linkOut, (EdgeType*), (override));

		MOCK_METHOD(void, unlinkIn, (EdgeType*), (override));
		MOCK_METHOD(void, unlinkOut, (EdgeType*), (override));

		virtual ~AbstractMockNode() {
		}

	private:
		void setUpGetters() {
			ON_CALL(*this, getId)
				.WillByDefault(ReturnPointee(&id));
			//EXPECT_CALL(*this, getId).Times(AnyNumber());

			ON_CALL(*this, getWeight)
				.WillByDefault(ReturnPointee(&weight));

		}
};

template<typename IdType>
using MockNode = AbstractMockNode<IdType, MockEdge<IdType>>;

#endif
