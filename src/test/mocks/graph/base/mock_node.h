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
	protected:
		IdType id;
		T _data;
		float weight = 1.;

	public:
		using typename FPMAS::api::graph::base::Node<
			T, IdType, ArcType>::arc_type;

		AbstractMockNode() {
			// Mock initialized without id
		}
		AbstractMockNode(const IdType& id) : id(id) {
			setUpGetters();
		}
		AbstractMockNode(const IdType& id, const T& data) : id(id), _data(data) {
			setUpGetters();
		}
		AbstractMockNode(const IdType& id, const T& data, float weight) : id(id), _data(data), weight(weight) {
			setUpGetters();
		}

		MOCK_METHOD(IdType, getId, (), (const, override));

		MOCK_METHOD(T&, data, (), (override));
		MOCK_METHOD(const T&, data, (), (const, override));

		MOCK_METHOD(float, getWeight, (), (const, override));
		MOCK_METHOD(void, setWeight, (float), (override));

		MOCK_METHOD(const std::vector<arc_type*>, getIncomingArcs, (), (override));
		MOCK_METHOD(const std::vector<arc_type*>, getIncomingArcs, (LayerId), (override));

		MOCK_METHOD(const std::vector<arc_type*>, getOutgoingArcs, (), (override));
		MOCK_METHOD(const std::vector<arc_type*>, getOutgoingArcs, (LayerId), (override));

		MOCK_METHOD(void, linkIn, (arc_type*), (override));
		MOCK_METHOD(void, linkOut, (arc_type*), (override));

		MOCK_METHOD(void, unlinkIn, (arc_type*), (override));
		MOCK_METHOD(void, unlinkOut, (arc_type*), (override));

	private:
		void setUpGetters() {
			ON_CALL(*this, getId)
				.WillByDefault(ReturnPointee(&id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());

			ON_CALL(*this, data())
				.WillByDefault(ReturnRef(_data));
			ON_CALL(Const(*this), data())
				.WillByDefault(ReturnRef(_data));

			ON_CALL(*this, getWeight)
				.WillByDefault(ReturnPointee(&weight));

		}
};


template<typename T, typename IdType>
class MockNode : public AbstractMockNode<T, IdType, MockArc<T, IdType>> {
	public:
		typedef AbstractMockNode<T, IdType, MockArc<T, IdType>> mock_node_base;
		MockNode() : mock_node_base() {}
		MockNode(const IdType& id)
			: mock_node_base(id) {
		}
		MockNode(const IdType& id, const T& data)
			: mock_node_base(id, std::move(data)) {
		}
		MockNode(const IdType& id, const T& data, float weight)
			: mock_node_base(id, std::move(data), weight) {
		}
};
#endif
