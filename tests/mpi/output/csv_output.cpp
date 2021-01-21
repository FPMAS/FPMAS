#include "fpmas/output/csv_output.h"
#include "fpmas/utils/macros.h"
#include "gtest/gtest.h"

TEST(DistributedCsvOutput, gather) {
	int size = fpmas::communication::WORLD.getSize();
	int i = size;
	std::string s = std::to_string(fpmas::communication::WORLD.getRank());
	std::ostringstream out;

	fpmas::output::DistributedCsvOutput<int, std::string> csv_out (
			fpmas::communication::WORLD, 0, out,
			{"field1", [&i] () {return i;}},
			{"field2", [&s] () {return s;}}
			);

	csv_out.dump();

	i = 0;
	s.clear();
	csv_out.dump();

	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		int expected_1 = size * size;
		std::string expected_2;
		for(int i = 0; i < size; i++)
			expected_2 += std::to_string(i);
		ASSERT_EQ(out.str(), 
				"field1,field2\n"
				+ std::to_string(expected_1) + "," + expected_2 + "\n"
				+ "0,\n");
	} else {
		ASSERT_EQ(out.str(), "");
	}
}

TEST(DistributedCsvOutput, all_gather) {
	int size = fpmas::communication::WORLD.getSize();
	int i = size;
	std::string s = std::to_string(fpmas::communication::WORLD.getRank());
	std::ostringstream out;

	fpmas::output::DistributedCsvOutput<int, std::string> csv_out (
			fpmas::communication::WORLD, out,
			{"field1", [&i] () {return i;}},
			{"field2", [&s] () {return s;}}
			);

	csv_out.dump();

	i = 0;
	s.clear();
	csv_out.dump();

	int expected_1 = size * size;
	std::string expected_2;
	for(int i = 0; i < size; i++)
		expected_2 += std::to_string(i);
	ASSERT_EQ(out.str(), 
			"field1,field2\n"
			+ std::to_string(expected_1) + "," + expected_2 + "\n"
			+ "0,\n");
}
