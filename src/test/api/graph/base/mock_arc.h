#ifndef MOCK_ARC_H
#define MOCK_ARC_H

#include "mock_node.h"
#include "api/graph/base/arc.h"

template<typename T, typename IdType>
class MockArc : public FPMAS::api::graph::base::Arc<T, IdType> {
	typedef FPMAS::api::graph::base::Node<T, IdType> node_type;
	public:
		MockArc() {}
		MockArc(IdType id, node_type* source, node_type* target, LayerId layer) {
			ON_CALL(*this, getId).WillByDefault(Return(id));
			ON_CALL(*this, getSourceNode).WillByDefault(Return(source));
			ON_CALL(*this, getTargetNode).WillByDefault(Return(target));
			ON_CALL(*this, getLayer).WillByDefault(Return(layer));
		}
		MockArc(IdType id, node_type* source, node_type* target, LayerId layer, float weight) {
			ON_CALL(*this, getId).WillByDefault(Return(id));
			ON_CALL(*this, getSourceNode).WillByDefault(Return(source));
			ON_CALL(*this, getTargetNode).WillByDefault(Return(target));
			ON_CALL(*this, getLayer).WillByDefault(Return(layer));
			ON_CALL(*this, getWeight).WillByDefault(Return(weight));
		}

		MOCK_METHOD(IdType, getId, (), (const, override));
		MOCK_METHOD(LayerId, getLayer, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(node_type* const, getSourceNode, (), (const, override));
		MOCK_METHOD(node_type* const, getTargetNode, (), (const, override));
};
#endif
