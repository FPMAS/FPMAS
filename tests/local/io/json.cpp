#include "fake_data.h"
#include "gmock/gmock.h"
#include "fpmas/graph/distributed_edge.h"
#include "graph/mock_distributed_node.h"

using namespace testing;

TEST(light_json, custom_rules) {
	NonDefaultConstructibleData data(8);

	// A custom light_json serialization rule is specified, so that nothing is
	// serialized...
	fpmas::io::json::light_json json = data;

	// ... and NonDefaultConstructibleData is initialized with 0
	NonDefaultConstructibleData unserialized_data = json.get<NonDefaultConstructibleData>();
	ASSERT_EQ(unserialized_data.i, 0);
}

TEST(light_json, default_constructible_nlohmann_fallback) {
	// nlohmann::adl_serializer fallback for default constructible data
	DefaultConstructibleSerializerOnly data;
	data.i = 12;

	fpmas::io::json::light_json j = data;

	DefaultConstructibleData unserialized_data = j.get<decltype(data)>();

	ASSERT_EQ(unserialized_data.i, 12);
}

TEST(light_json, non_default_constructible_nlohmann_fallback) {
	// nlohmann::adl_serializer fallback for non default constructible data
	NonDefaultConstructibleSerializerOnly data(12);

	fpmas::io::json::light_json j = data;

	NonDefaultConstructibleData unserialized_data = j.get<decltype(data)>();

	ASSERT_EQ(unserialized_data.i, 12);
}
