#ifndef MOCK_EDGE_H
#define MOCK_EDGE_H

#include "gmock/gmock.h"
#include "fpmas/api/graph/edge.h"

using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::SaveArg;
using fpmas::api::graph::LayerId;

template<typename IdType, typename> class AbstractMockNode;
//template<typename IdType> class MockNode;

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
		MOCK_METHOD(void, setLayer, (LayerId), (override));

		MOCK_METHOD(float, getWeight, (), (const));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(void, setSourceNode, (NodeType* const), (override));
		MOCK_METHOD(NodeType*, getSourceNode, (), (const, override));

		MOCK_METHOD(void, setTargetNode, (NodeType* const), (override));
		MOCK_METHOD(NodeType*, getTargetNode, (), (const, override));

	private:
		void setUpGetters() {
			ON_CALL(*this, getId).WillByDefault(ReturnPointee(&id));

			ON_CALL(*this, getLayer).WillByDefault(ReturnPointee(&layer));

			ON_CALL(*this, getWeight).WillByDefault(ReturnPointee(&weight));

			ON_CALL(*this, setSourceNode)
				// Saves to src
				.WillByDefault(SaveArg<0>(&src));
			ON_CALL(*this, getSourceNode)
				// Return the pointer pointed by the this->src field (the
				// pointed pointer might change during execution)
				.WillByDefault(ReturnPointee(const_cast<NodeType**>(&src)));

			// idem
			ON_CALL(*this, setTargetNode)
				.WillByDefault(SaveArg<0>(&tgt));
			ON_CALL(*this, getTargetNode)
				.WillByDefault(ReturnPointee(const_cast<NodeType**>(&tgt)));

		}
};

template<typename IdType> class MockEdge;
template<typename IdType>
using MockNode = AbstractMockNode<IdType, MockEdge<IdType>>;

template<typename IdType>
class MockEdge : public AbstractMockEdge<IdType, MockNode<IdType>> {
	public:
		using AbstractMockEdge<IdType, MockNode<IdType>>::AbstractMockEdge;
		//typedef AbstractMockEdge<IdType, MockNode<IdType>> mock_edge_base;
		//using typename mock_edge_base::NodeType;

		//MockEdge() : mock_edge_base() {}
		//MockEdge(IdType id, LayerId layer)
			//: mock_edge_base(id, layer) {
		//}
		//MockEdge(IdType id, LayerId layer, float weight)
			//: mock_edge_base(id, layer, weight) {
		//}
};
#endif
