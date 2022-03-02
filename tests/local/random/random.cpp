#include "fpmas/random/random.h"

#include "gmock/gmock.h"
#include <gmock/gmock-matchers.h>

using namespace testing;


/*
 * See tests/mpi/random/random.cpp for more relevant tests.
 */

TEST(Random, local_sample_n_sup_size) {
	std::vector<int> local_items(10);
	for(std::size_t i = 0; i < 10; i++)
		local_items[i] = i;

	std::vector<int> sample = fpmas::random::local_sample(local_items, 50);

	ASSERT_THAT(sample, SizeIs(local_items.size()));
	ASSERT_THAT(sample, UnorderedElementsAreArray(local_items));
}
