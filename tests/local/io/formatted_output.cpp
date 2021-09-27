#include "fpmas/io/formatted_output.h"

#include "gmock/gmock.h"

using namespace testing;

struct MockData {
	int data;

	MOCK_METHOD(void, out_operator, (), (const));
};

std::ostream& operator<<(std::ostream& out, const MockData& data) {
	data.out_operator();
	out << data.data;
	return out;
}

TEST(FormattedOutput, dump) {
	MockData mock_data;
	fpmas::io::StringOutput str_output;
	fpmas::io::FormattedOutput<const MockData&> output(
			str_output,
			[&mock_data] () -> const MockData& {return mock_data;}
			);

	mock_data.data=10;
	EXPECT_CALL(mock_data, out_operator);
	output.dump();

	mock_data.data=12;
	EXPECT_CALL(mock_data, out_operator);
	output.dump();
			
	ASSERT_EQ(str_output.str(), "1012");
}

TEST(FormattedOutput, dump_new_line) {
	NiceMock<MockData> mock_data;
	fpmas::io::StringOutput str_output;
	fpmas::io::FormattedOutput<const MockData&> output(
			str_output,
			[&mock_data] () -> const MockData& {return mock_data;},
			true
			);

	mock_data.data=10;
	output.dump();

	mock_data.data=12;
	output.dump();
			
	ASSERT_EQ(str_output.str(), "10\n12\n");
}
