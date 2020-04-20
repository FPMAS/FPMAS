#ifndef MOCK_ARC_H
#define MOCK_ARC_H

#include "gmock/gmock.h"
#include "api/graph/base/arc.h"

using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::SaveArg;
using FPMAS::api::graph::base::LayerId;

template<typename T, typename IdType, typename> class AbstractMockNode;
template<typename T, typename IdType> class MockNode;

template<typename T, typename IdType, typename NodeType>
class AbstractMockArc : public virtual FPMAS::api::graph::base::Arc<
				IdType, NodeType 
							> {
	public:
		using typename FPMAS::api::graph::base::Arc<IdType, NodeType>::node_type;
		using typename FPMAS::api::graph::base::Arc<IdType, NodeType>::id_type;

		/*
		 * Expectations obvisouly can't be set on an object that haven't been constructed
		 * yet. But the Graph for example will instantiate Arcs, and so
		 * MockArcs in this case, internally. But we still want to check some
		 * properties of those arcs that can be set :
		 * - when the arc is constructed (id, layer...)
		 * - using setters (setState...)
		 * To solve this issue, the general approach is :
		 * - expect each function call AnyBumber() of times
		 * - when setters (and constructors) are called, save the value to
		 *   internal fields (id, src, tgt...)
		 * - when getters are called, return the saved values
		 */
		IdType id;
		const node_type* src;
		const node_type* tgt;

		AbstractMockArc() {
			EXPECT_CALL(*this, setSourceNode)
				.Times(AnyNumber())
				// Saves to src
				.WillRepeatedly(SaveArg<0>(&src));
			EXPECT_CALL(*this, getSourceNode)
				.Times(AnyNumber())
				// Return the pointer pointed by the this->src field (the
				// pointed pointer might change during execution)
				.WillRepeatedly(ReturnPointee(const_cast<node_type**>(&src)));

			// idem
			EXPECT_CALL(*this, setTargetNode)
				.Times(AnyNumber())
				.WillRepeatedly(SaveArg<0>(&tgt));
			EXPECT_CALL(*this, getTargetNode)
				.Times(AnyNumber())
				.WillRepeatedly(ReturnPointee(const_cast<node_type**>(&tgt)));
		}
		AbstractMockArc(IdType id, LayerId layer) : AbstractMockArc() {
			this->id = id;
			ON_CALL(*this, getId).WillByDefault(Return(id));

			EXPECT_CALL(*this, getId).Times(AnyNumber());
			ON_CALL(*this, getLayer).WillByDefault(Return(layer));
		}
		AbstractMockArc(IdType id, LayerId layer, float weight)
			: AbstractMockArc(id, layer) {
			ON_CALL(*this, getWeight).WillByDefault(Return(weight));
		}

		MOCK_METHOD(id_type, getId, (), (const, override));
		MOCK_METHOD(LayerId, getLayer, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(void, setSourceNode, (node_type* const), (override));
		MOCK_METHOD(node_type* const, getSourceNode, (), (const, override));

		MOCK_METHOD(void, setTargetNode, (node_type* const), (override));
		MOCK_METHOD(node_type* const, getTargetNode, (), (const, override));
};

template<typename T, typename IdType>
class MockArc : public AbstractMockArc<T, IdType, MockNode<T, IdType>> {
	public:
		typedef AbstractMockArc<T, IdType, MockNode<T, IdType>> mock_arc_base;
		using typename mock_arc_base::node_type;

		MockArc() : mock_arc_base() {}
		MockArc(IdType id, LayerId layer)
			: mock_arc_base(id, layer) {
		}
		MockArc(IdType id, LayerId layer, float weight)
			: mock_arc_base(id, layer, weight) {
		}
};
#endif
