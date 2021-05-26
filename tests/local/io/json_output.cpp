#include "fpmas/io/json_output.h"
#include "gmock/gmock.h"

using namespace testing;

TEST(JsonOutput, dump_test) {
	std::unordered_map<int, std::string> data = {
		{8, "hello"},
		{-2, "world"}
	};

	fpmas::io::StringOutput output_str;
	fpmas::io::JsonOutput<decltype(data)> json_output(
			output_str, [&data] () {return data;}
			);
	json_output.dump();

	ASSERT_THAT(
			nlohmann::json::parse(output_str.str()).get<decltype(data)>(),
			UnorderedElementsAreArray(data)
			);
}
