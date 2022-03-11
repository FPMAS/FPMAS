#include "fpmas/random/random.h"
#include "fpmas/random/distribution.h"

#include "gmock/gmock.h"
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(Random, generator_iostream) {
	fpmas::random::mt19937_64 gen(624);

	std::stringstream str_stream;
	str_stream << gen << std::endl;

	fpmas::random::mt19937_64 unserial_gen;
	str_stream >> unserial_gen;

	ASSERT_EQ(gen, unserial_gen);
}

TEST(Random, generator_json) {
	fpmas::random::mt19937_64 gen(624);

	nlohmann::json j = gen;
	fpmas::random::mt19937_64 unserial_gen = j.get<fpmas::random::mt19937_64>();

	ASSERT_EQ(gen, unserial_gen);
}

TEST(Random, generator_datapack) {
	fpmas::random::mt19937_64 gen(624);

	fpmas::io::datapack::ObjectPack pack = gen;
	fpmas::random::mt19937_64 unserial_gen = pack.get<fpmas::random::mt19937_64>();

	ASSERT_EQ(gen, unserial_gen);
}

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

struct dist_index_cmp {
	bool operator()(
			const fpmas::random::DistributedIndex& i1,
			const fpmas::random::DistributedIndex& i2) const {
		if(i1.key() == i2.key())
			return i1.offset() < i2.offset();
		return i1.key() < i2.key();
	}
};

TEST(Random, distributed_index) {
	using fpmas::random::DistributedIndex;

	std::vector<std::size_t> sizes(30);

	fpmas::random::mt19937_64 gen;
	fpmas::random::UniformIntDistribution<> rd_size(1, 20);
	std::size_t total_count = 0;
	for(std::size_t i = 0; i < sizes.size(); i++) {
		sizes[i] = rd_size(gen);
		total_count+=sizes[i];
	}
	// Sets some random sizes to 0
	for(std::size_t i : fpmas::random::sample_indexes(0ul, 30ul, 5, gen)) {
		total_count-=sizes[i];
		sizes[i] = 0;
	}
	// Sets last item to 0
	total_count-=sizes[29];
	sizes[29] = 0;

	DistributedIndex dist_index_begin = DistributedIndex::begin(sizes);
	DistributedIndex dist_index_end = DistributedIndex::end(sizes);

	std::set<fpmas::random::DistributedIndex, dist_index_cmp> value_set;
	std::vector<std::size_t> expected_offsets;
	for(auto item : sizes)
		for(std::size_t i = 0; i < item; i++)
			expected_offsets.push_back(i);

	std::size_t count = 0;
	while(dist_index_begin != dist_index_end) {
		value_set.insert(dist_index_begin);
		ASSERT_THAT(dist_index_begin.offset(), AllOf(Ge(0), Lt(sizes[dist_index_begin.key()])));
		ASSERT_EQ(dist_index_begin.offset(), expected_offsets[count]);
		dist_index_begin++;
		count++;
	}
	ASSERT_EQ(count, total_count);
	// All values are different
	ASSERT_THAT(value_set, SizeIs(total_count));
}

TEST(Random, distributed_index_random_increment) {
	std::vector<std::size_t> item_counts {{
		0, 3, 7, 0, 2, 8, 10
	}};
	using fpmas::random::DistributedIndex;

	DistributedIndex begin = DistributedIndex::begin(item_counts);

	DistributedIndex index = begin+2;
	ASSERT_EQ(index.key(), 1);
	ASSERT_EQ(index.offset(), 2);

	index = begin+4;
	ASSERT_EQ(index.key(), 2);
	ASSERT_EQ(index.offset(), 1);

	index = begin+15;
	ASSERT_EQ(index.key(), 5);
	ASSERT_EQ(index.offset(), 3);

	index = begin+10;
	ASSERT_EQ(index.key(), 4);
	ASSERT_EQ(index.offset(), 0);
}

TEST(Random, distributed_index_distance) {
	std::vector<std::size_t> item_counts {{
		0, 3, 7, 0, 2, 8, 10
	}};
	using fpmas::random::DistributedIndex;

	DistributedIndex begin = DistributedIndex::begin(item_counts);
	DistributedIndex end = DistributedIndex::end(item_counts);

	ASSERT_EQ(DistributedIndex::distance(begin, begin+2), 2);
	ASSERT_EQ(DistributedIndex::distance(begin, begin+11), 11);
	ASSERT_EQ(DistributedIndex::distance(begin+4, begin+20), 16);
	ASSERT_EQ(DistributedIndex::distance(begin, end), 3+7+2+8+10);
}

