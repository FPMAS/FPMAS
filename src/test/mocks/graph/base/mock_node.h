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

using fpmas::api::graph::base::Id;
using fpmas::api::graph::base::LayerId;

class MockData {};

template<typename IdType, typename _ArcType>
class AbstractMockNode : public virtual fpmas::api::graph::base::Node<
				 IdType, _ArcType
							 > {
	protected:
		IdType id;
		float weight = 1.;

	public:
		using typename fpmas::api::graph::base::Node<
			IdType, _ArcType>::ArcType;

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

		MOCK_METHOD(const std::vector<ArcType*>, getIncomingArcs, (), (const, override));
		MOCK_METHOD(const std::vector<ArcType*>, getIncomingArcs, (LayerId), (const, override));

		MOCK_METHOD(const std::vector<ArcType*>, getOutgoingArcs, (), (const, override));
		MOCK_METHOD(const std::vector<ArcType*>, getOutgoingArcs, (LayerId), (const, override));

		MOCK_METHOD(void, linkIn, (ArcType*), (override));
		MOCK_METHOD(void, linkOut, (ArcType*), (override));

		MOCK_METHOD(void, unlinkIn, (ArcType*), (override));
		MOCK_METHOD(void, unlinkOut, (ArcType*), (override));

		virtual ~AbstractMockNode() {}

	private:
		void setUpGetters() {
			ON_CALL(*this, getId)
				.WillByDefault(ReturnPointee(&id));
			EXPECT_CALL(*this, getId).Times(AnyNumber());

			ON_CALL(*this, getWeight)
				.WillByDefault(ReturnPointee(&weight));

		}
};


template<typename IdType>
class MockNode : public AbstractMockNode<IdType, MockArc<IdType>> {
	public:
		typedef AbstractMockNode<IdType, MockArc<IdType>> MockNodeBase;
		MockNode() : MockNodeBase() {
			EXPECT_CALL(*this, die).Times(AnyNumber());
		}
		MockNode(const IdType& id)
			: MockNodeBase(id) {
			EXPECT_CALL(*this, die).Times(AnyNumber());
		}
		MockNode(const IdType& id, float weight)
			: MockNodeBase(id, weight) {
			EXPECT_CALL(*this, die).Times(AnyNumber());
		}

		MOCK_METHOD(void, die, (), ());
		~MockNode() {die();}
};
#endif
