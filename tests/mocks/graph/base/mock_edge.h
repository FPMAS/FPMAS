#ifndef MOCK_EDGE_H
#define MOCK_EDGE_H

#include "gmock/gmock.h"
#include "fpmas/api/graph/base/edge.h"

using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::SaveArg;
using fpmas::api::graph::LayerId;

template<typename IdType, typename> class AbstractMockNode;
template<typename IdType> class MockNode;

template<typename _IdType, typename _NodeType>
class AbstractMockEdge : public virtual fpmas::api::graph::Edge<
				_IdType, _NodeType 
							> {
	public:
		using typename fpmas::api::graph::Edge<_IdType, _NodeType>::NodeType;
		using typename fpmas::api::graph::Edge<_IdType, _NodeType>::IdType;

		/*
		 * Expectations obvisouly can't be set on an object that haven't been constructed
		 * yet. But the Graph for example will instantiate Edges, and so
		 * MockEdges in this case, internally. But we still want to check some
		 * properties of those edges that can be set :
		 * - when the edge is constructed (id, layer...)
		 * - using setters (setState...)
		 * To solve this issue, the general approach is :
		 * - expect each function call AnyBumber() of times
		 * - when setters (and constructors) are called, save the value to
		 *   internal fields (id, src, tgt...)
		 * - when getters are called, return the saved values
		 */
		IdType id;
		const NodeType* src;
		const NodeType* tgt;
		LayerId layer;
		float weight;

		AbstractMockEdge() {
			setUpGetters();
		}
		AbstractMockEdge(IdType id, LayerId layer) : id(id), layer(layer) {
			setUpGetters();
		}
		AbstractMockEdge(IdType id, LayerId layer, float weight)
			: id(id), layer(layer), weight(weight) {
			setUpGetters();
		}

		MOCK_METHOD(IdType, getId, (), (const, override));
		MOCK_METHOD(LayerId, getLayer, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(void, setSourceNode, (NodeType* const), (override));
		MOCK_METHOD(NodeType* const, getSourceNode, (), (const, override));

		MOCK_METHOD(void, setTargetNode, (NodeType* const), (override));
		MOCK_METHOD(NodeType* const, getTargetNode, (), (const, override));

	private:
		void setUpGetters() {
			ON_CALL(*this, getId).WillByDefault(ReturnPointee(&id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());

			ON_CALL(*this, getLayer).WillByDefault(ReturnPointee(&layer));
			EXPECT_CALL(*this, getLayer).Times(AnyNumber());

			ON_CALL(*this, getWeight).WillByDefault(ReturnPointee(&weight));
			EXPECT_CALL(*this, getWeight).Times(AnyNumber());

			EXPECT_CALL(*this, setSourceNode)
				.Times(AnyNumber())
				// Saves to src
				.WillRepeatedly(SaveArg<0>(&src));
			EXPECT_CALL(*this, getSourceNode)
				.Times(AnyNumber())
				// Return the pointer pointed by the this->src field (the
				// pointed pointer might change during execution)
				.WillRepeatedly(ReturnPointee(const_cast<NodeType**>(&src)));

			// idem
			EXPECT_CALL(*this, setTargetNode)
				.Times(AnyNumber())
				.WillRepeatedly(SaveArg<0>(&tgt));
			EXPECT_CALL(*this, getTargetNode)
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(const_cast<NodeType**>(&tgt)));

		}
};

template<typename IdType>
class MockEdge : public AbstractMockEdge<IdType, MockNode<IdType>> {
	public:
		typedef AbstractMockEdge<IdType, MockNode<IdType>> mock_edge_base;
		using typename mock_edge_base::NodeType;

		MockEdge() : mock_edge_base() {}
		MockEdge(IdType id, LayerId layer)
			: mock_edge_base(id, layer) {
		}
		MockEdge(IdType id, LayerId layer, float weight)
			: mock_edge_base(id, layer, weight) {
		}
};
#endif
