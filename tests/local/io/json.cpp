#include "fake_data.h"
#include "gmock/gmock.h"
#include "fpmas/graph/distributed_edge.h"
#include "graph/mock_distributed_node.h"
#include "../gtest_environment.h"

using namespace testing;

TEST(light_json, non_default_constructible) {
	NonDefaultConstructibleData data(8);

	fpmas::io::json::light_json json = data;

	NonDefaultConstructibleData unserialized_data = json.get<NonDefaultConstructibleData>();
	ASSERT_EQ(unserialized_data.i, 0);
}

TEST(light_json, default_constructible_adl_only) {
	DefaultConstructibleAdlOnly data;
	data.i = 12;

	fpmas::io::json::light_json j = data;

	DefaultConstructibleData unserialized_data = j.get<decltype(data)>();

	ASSERT_EQ(unserialized_data.i, 12);
}

TEST(light_json, non_default_constructible_adl_only) {
	NonDefaultConstructibleAdlOnly data(12);

	fpmas::io::json::light_json j = data;

	NonDefaultConstructibleData unserialized_data = j.get<decltype(data)>();

	ASSERT_EQ(unserialized_data.i, 12);
}

TEST(light_json, distributed_edge) {
	fpmas::graph::DistributedEdge<DefaultConstructibleData> edge {{2, 6}, 7};
	MockDistributedNode<DefaultConstructibleData, NiceMock> src {{0, 1}, {}, 0.7};
	src.data().i = 4;
	MockDistributedNode<DefaultConstructibleData, NiceMock> tgt {{0, 2}, {}, 2.45};
	tgt.data().i = 13;
	ON_CALL(src, location)
		.WillByDefault(Return(3));
	ON_CALL(tgt, location)
		.WillByDefault(Return(22));

	edge.setWeight(2.4);
	edge.setSourceNode(&src);
	edge.setTargetNode(&tgt);

	auto edge_ptr = fpmas::graph::EdgePtrWrapper<DefaultConstructibleData>(&edge);
	nlohmann::json classic_json = edge_ptr;
	fpmas::io::json::light_json light_json = edge_ptr;

	ASSERT_LT(light_json.dump().size(), classic_json.dump().size());

	edge_ptr = light_json.get<decltype(edge_ptr)>();
	ASSERT_EQ(edge_ptr->getId(), edge.getId());
	ASSERT_FLOAT_EQ(edge_ptr->getWeight(), edge.getWeight());

	auto temp_src = edge_ptr->getTempSourceNode();
	ASSERT_EQ(temp_src->getId(), src.getId());
	ASSERT_EQ(temp_src->getLocation(), 3);
	auto built_src = temp_src->build();
	ASSERT_EQ(built_src->getId(), src.getId());
	ASSERT_EQ(built_src->location(), 3);

	auto temp_tgt = edge_ptr->getTempTargetNode();
	ASSERT_EQ(temp_tgt->getId(), tgt.getId());
	ASSERT_EQ(temp_tgt->getLocation(), 22);
	auto built_tgt = temp_tgt->build();
	ASSERT_EQ(built_tgt->getId(), tgt.getId());
	ASSERT_EQ(built_tgt->location(), 22);

	delete built_src;
	delete built_tgt;
	delete edge_ptr.get();
}

TEST(light_json, agent_ptr_default_constructible) {
	fpmas::model::AgentPtr ptr(new model::test::GridAgentWithData(4));

	nlohmann::json classic_json = ptr;
	fpmas::io::json::light_json light_json = ptr;

	ASSERT_LT(light_json.dump().size(), classic_json.dump().size());

	fpmas::model::AgentPtr unserialized_agent = light_json.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<model::test::GridAgentWithData*>(Not(IsNull()))
			);
}

TEST(light_json, agent_ptr_not_default_constructible) {
	fpmas::model::AgentPtr ptr(new model::test::SpatialAgentWithData(4));

	fpmas::io::json::light_json light_json = ptr;
	fpmas::model::AgentPtr unserialized_agent = light_json.get<fpmas::model::AgentPtr>();

	ASSERT_THAT(
			unserialized_agent.get(),
			WhenDynamicCastTo<model::test::SpatialAgentWithData*>(Not(IsNull()))
			);

	// Ensures that adl_serializer was used
	ASSERT_THAT(
			dynamic_cast<model::test::SpatialAgentWithData*>(unserialized_agent.get())->data,
			4
			);
}
