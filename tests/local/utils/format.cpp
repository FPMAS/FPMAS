#include "fpmas/utils/format.h"

#include "gtest/gtest.h"

TEST(Format, generic) {
	std::string input("%lhello %l");
	input = fpmas::utils::format(input, "%l", 4);

	ASSERT_EQ(input, "4hello 4");
}

TEST(Format, rank) {
	std::string input("hello%r %r");
	input = fpmas::utils::format(input, 2);

	ASSERT_EQ(input, "hello2 2");
}

TEST(Format, time) {
	std::string input("time: %t, %t");
	input = fpmas::utils::format(input, (fpmas::api::scheduler::TimeStep) 10.0);

	ASSERT_EQ(input, "time: 10, 10");
}

TEST(Format, rank_time) {
	std::string input(" %t: %r, %t  %r");
	input = fpmas::utils::format(input, 2, 12);

	ASSERT_EQ(input, " 12: 2, 12  2");
}
