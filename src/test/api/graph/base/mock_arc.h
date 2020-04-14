#ifndef MOCK_ARC_H
#define MOCK_ARC_H

#include "gmock/gmock.h"
#include "api/graph/base/arc.h"

using ::testing::Return;
using ::testing::AnyNumber;
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

		AbstractMockArc() {}
		AbstractMockArc(IdType id, node_type* source, node_type* target, LayerId layer) {
			ON_CALL(*this, getId).WillByDefault(Return(id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());
			ON_CALL(*this, getSourceNode).WillByDefault(Return(source));
			ON_CALL(*this, getTargetNode).WillByDefault(Return(target));
			ON_CALL(*this, getLayer).WillByDefault(Return(layer));
		}
		AbstractMockArc(IdType id, node_type* source, node_type* target, LayerId layer, float weight) {
			ON_CALL(*this, getId).WillByDefault(Return(id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());
			ON_CALL(*this, getSourceNode).WillByDefault(Return(source));
			ON_CALL(*this, getTargetNode).WillByDefault(Return(target));
			ON_CALL(*this, getLayer).WillByDefault(Return(layer));
			ON_CALL(*this, getWeight).WillByDefault(Return(weight));
		}

		MOCK_METHOD(id_type, getId, (), (const, override));
		MOCK_METHOD(LayerId, getLayer, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(node_type* const, getSourceNode, (), (const, override));
		MOCK_METHOD(node_type* const, getTargetNode, (), (const, override));

		MOCK_METHOD(void, unlink, (), (override));
};

template<typename T, typename IdType>
class MockArc : public AbstractMockArc<T, IdType, MockNode<T, IdType>> {
	public:
		typedef AbstractMockArc<T, IdType, MockNode<T, IdType>> mock_arc_base;
		using typename mock_arc_base::node_type;

		MockArc() : mock_arc_base() {}
		MockArc(IdType id, node_type* source, node_type* target, LayerId layer)
			: mock_arc_base(source, target, layer) {
		}
		MockArc(IdType id, node_type* source, node_type* target, LayerId layer, float weight)
			: mock_arc_base(id, source, target, layer, weight) {
		}
};

/*
 *template<typename T, typename IdType>
 *class MockArc : public virtual FPMAS::api::graph::base::Arc<
 *                IdType, MockNode<T, IdType> 
 *                            > {
 *    public:
 *        using typename FPMAS::api::graph::base::Arc<IdType, MockNode<T, IdType>>::node_type;
 *        using typename FPMAS::api::graph::base::Arc<IdType, MockNode<T, IdType>>::id_type;
 *
 *        MockArc() {}
 *        MockArc(IdType id, node_type* source, node_type* target, LayerId layer) {
 *            ON_CALL(*this, getId).WillByDefault(Return(id));
 *            ON_CALL(*this, getSourceNode).WillByDefault(Return(source));
 *            ON_CALL(*this, getTargetNode).WillByDefault(Return(target));
 *            ON_CALL(*this, getLayer).WillByDefault(Return(layer));
 *        }
 *        MockArc(IdType id, node_type* source, node_type* target, LayerId layer, float weight) {
 *            ON_CALL(*this, getId).WillByDefault(Return(id));
 *            ON_CALL(*this, getSourceNode).WillByDefault(Return(source));
 *            ON_CALL(*this, getTargetNode).WillByDefault(Return(target));
 *            ON_CALL(*this, getLayer).WillByDefault(Return(layer));
 *            ON_CALL(*this, getWeight).WillByDefault(Return(weight));
 *        }
 *
 *        MOCK_METHOD(id_type, getId, (), (const, override));
 *        MOCK_METHOD(LayerId, getLayer, (), (const, override));
 *
 *        MOCK_METHOD(float, getWeight, (), (const));
 *        MOCK_METHOD(void, setWeight, (float), (override));
 *
 *        MOCK_METHOD(node_type* const, getSourceNode, (), (const, override));
 *        MOCK_METHOD(node_type* const, getTargetNode, (), (const, override));
 *
 *        MOCK_METHOD(void, unlink, (), (override));
 *};
 */
#endif
