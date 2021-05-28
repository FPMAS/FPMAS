#include "fpmas/utils/functional.h"

#include "gmock/gmock.h"

using namespace testing;

TEST(Concat, test) {
	fpmas::utils::Concat concat;

	std::vector<int> v1 {{1, 2, 2, 4}};
	std::vector<int> v2 {{6, 2}};

	std::vector<int> v = concat(v1, v2);

	ASSERT_THAT(v, ElementsAre(1, 2, 2, 4, 6, 2));
}
