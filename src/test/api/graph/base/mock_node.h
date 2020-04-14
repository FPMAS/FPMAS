#ifndef MOCK_NODE_H
#define MOCK_NODE_H

#include "gmock/gmock.h"

#include "api/graph/base/id.h"
#include "api/graph/base/node.h"
#include "mock_arc.h"

using ::testing::Const;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::AnyNumber;

using FPMAS::api::graph::base::Id;
using FPMAS::api::graph::base::LayerId;

class MockData {};

template<typename T, typename IdType, typename ArcType>
class AbstractMockNode : public virtual FPMAS::api::graph::base::Node<
				 T, IdType, ArcType
							 > {
	private:
		T _data;
	public:
		using typename FPMAS::api::graph::base::Node<
			T, IdType, ArcType>::arc_type;

		AbstractMockNode() {}
		AbstractMockNode(const IdType& id) {
			ON_CALL(*this, getId)
				.WillByDefault(Return(id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());
		}
		AbstractMockNode(const IdType& id, T&& data) : _data(data) {
			ON_CALL(*this, getId)
				.WillByDefault(Return(id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());

			ON_CALL(*this, data())
				.WillByDefault(ReturnRef(_data));

			ON_CALL(Const(*this), data())
				.WillByDefault(ReturnRef(_data));
		}
		AbstractMockNode(const IdType& id, T&& data, float weight) : _data(data) {
			ON_CALL(*this, getId)
				.WillByDefault(Return(id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());

			ON_CALL(*this, data())
				.WillByDefault(ReturnRef(_data));

			ON_CALL(Const(*this), data())
				.WillByDefault(ReturnRef(_data));

			ON_CALL(*this, getWeight)
				.WillByDefault(Return(weight));
		}

		MOCK_METHOD(IdType, getId, (), (const, override));

		MOCK_METHOD(T&, data, (), (override));
		MOCK_METHOD(const T&, data, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const, override));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(std::vector<arc_type*>, getIncomingArcs, (), (override));
		MOCK_METHOD(std::vector<arc_type*>, getIncomingArcs, (LayerId), (override));

		MOCK_METHOD(std::vector<arc_type*>, getOutgoingArcs, (), (override));
		MOCK_METHOD(std::vector<arc_type*>, getOutgoingArcs, (LayerId), (override));

		MOCK_METHOD(void, linkIn, (arc_type*, LayerId), (override));
		MOCK_METHOD(void, linkOut, (arc_type*, LayerId), (override));

		MOCK_METHOD(void, unlinkIn, (arc_type*, LayerId), (override));
		MOCK_METHOD(void, unlinkOut, (arc_type*, LayerId), (override));
};


template<typename T, typename IdType>
class MockNode : public AbstractMockNode<T, IdType, MockArc<T, IdType>> {
	public:
		typedef AbstractMockNode<T, IdType, MockArc<T, IdType>> mock_node_base;
		MockNode() : mock_node_base() {}
		MockNode(const IdType& id)
			: mock_node_base(id) {
		}
		MockNode(const IdType& id, T&& data)
			: mock_node_base(id, std::move(data)) {
		}
		MockNode(const IdType& id, T&& data, float weight)
			: mock_node_base(id, std::move(data), weight) {
		}
};
/*
 *
 *template<typename T, typename IdType>
 *class MockNode : public virtual FPMAS::api::graph::base::Node<
 *                 T, IdType, MockArc<T, IdType>
 *                             > {
 *    public:
 *        using typename FPMAS::api::graph::base::Node<
 *            T, IdType, MockArc<T, IdType>>::arc_type;
 *
 *        MockNode() {}
 *        MockNode(const IdType& id) {
 *            ON_CALL(*this, getId)
 *                .WillByDefault(Return(id));
 *        }
 *        MockNode(const IdType& id, T&& data) {
 *            ON_CALL(*this, getId)
 *                .WillByDefault(Return(id));
 *            ON_CALL(*this, data())
 *                .WillByDefault(ReturnRef(data));
 *            ON_CALL(Const(*this), data())
 *                .WillByDefault(ReturnRef(data));
 *        }
 *        MockNode(const IdType& id, T&& data, float weight) {
 *            ON_CALL(*this, getId)
 *                .WillByDefault(Return(id));
 *            ON_CALL(*this, data())
 *                .WillByDefault(ReturnRef(data));
 *            ON_CALL(Const(*this), data())
 *                .WillByDefault(ReturnRef(data));
 *            ON_CALL(*this, getWeight)
 *                .WillByDefault(Return(weight));
 *        }
 *
 *        MOCK_METHOD(IdType, getId, (), (const, override));
 *
 *        MOCK_METHOD(T&, data, (), (override));
 *        MOCK_METHOD(const T&, data, (), (const, override));
 *
 *        MOCK_METHOD(float, getWeight, (), (const, override));
 *        MOCK_METHOD(void, setWeight, (float), (override));
 *
 *        MOCK_METHOD(std::vector<arc_type*>, getIncomingArcs, (), (override));
 *        MOCK_METHOD(std::vector<arc_type*>, getIncomingArcs, (LayerId), (override));
 *
 *        MOCK_METHOD(std::vector<arc_type*>, getOutgoingArcs, (), (override));
 *        MOCK_METHOD(std::vector<arc_type*>, getOutgoingArcs, (LayerId), (override));
 *
 *        MOCK_METHOD(void, linkIn, (arc_type*, LayerId), (override));
 *        MOCK_METHOD(void, linkOut, (arc_type*, LayerId), (override));
 *
 *        MOCK_METHOD(void, unlinkIn, (arc_type*, LayerId), (override));
 *        MOCK_METHOD(void, unlinkOut, (arc_type*, LayerId), (override));
 *};
 */
#endif
