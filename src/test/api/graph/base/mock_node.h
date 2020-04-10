#ifndef MOCK_NODE_H
#define MOCK_NODE_H

#include "gmock/gmock.h"

#include "api/graph/base/id.h"
#include "api/graph/base/node.h"

using ::testing::Const;
using ::testing::Return;
using ::testing::ReturnRef;

using FPMAS::api::graph::base::Id;
using FPMAS::api::graph::base::LayerId;

class MockData {};

template<typename T, typename IdType>
class MockNode : public FPMAS::api::graph::base::Node<T, IdType> {
	public:
		typedef FPMAS::api::graph::base::Arc<T, IdType>* arc_ptr;

		MockNode() {}
		MockNode(const IdType& id) {
			ON_CALL(*this, getId)
				.WillByDefault(Return(id));
		}
		MockNode(const IdType& id, T&& data) {
			ON_CALL(*this, getId)
				.WillByDefault(Return(id));
			ON_CALL(*this, data())
				.WillByDefault(ReturnRef(data));
			ON_CALL(Const(*this), data())
				.WillByDefault(ReturnRef(data));
		}
		MockNode(const IdType& id, T&& data, float weight) {
			ON_CALL(*this, getId)
				.WillByDefault(Return(id));
			ON_CALL(*this, data())
				.WillByDefault(ReturnRef(data));
			ON_CALL(Const(*this), data())
				.WillByDefault(ReturnRef(data));
			ON_CALL(*this, getWeight)
				.WillByDefault(Return(weight));
		}

		MOCK_METHOD(IdType, getId, (), (const, override));

		MOCK_METHOD(T&, data, (), (override));
		MOCK_METHOD(const T&, data, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const, override));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(std::vector<arc_ptr>, getIncomingArcs, (), (override));
		MOCK_METHOD(std::vector<arc_ptr>, getIncomingArcs, (LayerId), (override));

		MOCK_METHOD(std::vector<arc_ptr>, getOutgoingArcs, (), (override));
		MOCK_METHOD(std::vector<arc_ptr>, getOutgoingArcs, (LayerId), (override));

		MOCK_METHOD(void, linkIn, (arc_ptr, LayerId), (override));
		MOCK_METHOD(void, linkOut, (arc_ptr, LayerId), (override));

		MOCK_METHOD(void, unlinkIn, (arc_ptr, LayerId), (override));
		MOCK_METHOD(void, unlinkOut, (arc_ptr, LayerId), (override));
};
#endif
