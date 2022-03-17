#include "fpmas/random/random.h"
#include "fpmas/communication/communication.h"
#include "fpmas/random/distribution.h"
#include "fpmas/utils/functional.h"
#include "fpmas/utils/macros.h"
#include "graph/mock_distributed_node.h"
#include <algorithm>
#include <gmock/gmock-matchers.h>
#include <numeric>
#include <unordered_set>

#include "gmock/gmock.h"

using namespace testing;

TEST(Random, local_choices_sequential_gen) {
	std::vector<int> local_items;
	fpmas::random::mt19937_64 gen;
	fpmas::random::UniformIntDistribution<int> int_dist(0, 99);

	for(std::size_t i = 0; i < 50; i++)
		local_items.push_back(int_dist(gen));

	std::vector<int> local_choices
		= fpmas::random::local_choices(local_items, 10000, gen);
	std::vector<int> histogram(100);
	for(std::size_t item : local_choices)
		histogram[item]++;
	for(auto item : histogram)
		ASSERT_NEAR((float) item / 10000, 1.f/100, 0.1);

	fpmas::communication::TypedMpi<std::vector<int>> vector_id_mpi(fpmas::communication::WORLD);
	std::vector<std::vector<int>> global_choices
		= vector_id_mpi.gather(local_choices, 0);

	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		for(std::size_t i = 1; i < global_choices.size(); i++)
			ASSERT_THAT(global_choices[i], ElementsAreArray(global_choices[0]));
	}
}

TEST(Random, local_choices_distributed_gen) {
	std::vector<int> local_items;
	fpmas::random::mt19937_64 gen;
	fpmas::random::UniformIntDistribution<int> int_dist(0, 99);

	for(std::size_t i = 0; i < 50; i++)
		local_items.push_back(int_dist(gen));

	std::vector<int> local_choices
		= fpmas::random::local_choices(local_items, 10000);

	std::vector<int> histogram(100);
	for(auto item : local_choices)
		histogram[item]++;
	for(auto item : histogram)
		ASSERT_NEAR((float) item / 10000, 1.f/100, 0.1);

	fpmas::communication::TypedMpi<std::vector<int>> vector_id_mpi(fpmas::communication::WORLD);
	std::vector<std::vector<int>> global_choices
		= vector_id_mpi.gather(local_choices, 0);

	FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
		for(std::size_t i = 1; i < global_choices.size(); i++)
			for(std::size_t j = 0; j < i; j++)
				ASSERT_THAT(global_choices[i], Not(ElementsAreArray(global_choices[j])));
	}
}

TEST(Random, distributed_choices) {
	static const std::size_t N_LOCAL_ITEMS = 100000;
	static const std::size_t N_CHOICES = 5*N_LOCAL_ITEMS;

	fpmas::random::DistributedGenerator<> gen;
	fpmas::random::UniformIntDistribution<int> int_dist(0, 99);

	// Local items are pair of values such as `first=current rank` and
	// `second=uniformly selected integer in [0, 100]`
	std::vector<std::pair<int, int>> local_items;
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items.push_back({fpmas::communication::WORLD.getRank(), int_dist(gen)});

	// Randomly choses 50000 items on each process from all processes
	auto gather_choices = fpmas::random::distributed_choices(
			fpmas::communication::WORLD, local_items, N_CHOICES);

	// Accumulates results in a single vector
	std::vector<std::pair<int, int>> choices = std::accumulate(
			gather_choices.begin(), gather_choices.end(), std::vector<std::pair<int, int>>(),
			fpmas::utils::Concat());

	ASSERT_THAT(choices, SizeIs(N_CHOICES));

	std::vector<int> process_histogram(fpmas::communication::WORLD.getSize());
	std::vector<std::vector<int>> value_histograms(fpmas::communication::WORLD.getSize());
	for(int i = 0; i < fpmas::communication::WORLD.getSize(); i++)
		value_histograms[i].resize(100);

	for(auto item : choices) {
		process_histogram[item.first]++;
		value_histograms[item.first][item.second]++;
	}

	// Ensures that values are uniformly selected from all processes
	for(auto item : process_histogram)
		ASSERT_NEAR((float) item / N_CHOICES, 1.f / fpmas::communication::WORLD.getSize(), 0.1);

	// Ensures that values are uniformly selected on each process
	for(int rank = 0; rank < fpmas::communication::WORLD.getSize(); rank++) {
		for(auto item : value_histograms[rank])
			ASSERT_NEAR((float) item / gather_choices[rank].size(), 1.f/100, 0.1f);
	}
}

TEST(Random, split_choices) {
	static const std::size_t N_LOCAL_ITEMS = 100000;
	static const std::size_t N_CHOICES = 5*N_LOCAL_ITEMS;

	fpmas::random::DistributedGenerator<> gen;
	fpmas::random::UniformIntDistribution<int> int_dist(0, 99);

	// Local items are pair of values such as `first=current rank` and
	// `second=uniformly selected integer in [0, 100]`
	std::vector<std::pair<int, int>> local_items;
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items.push_back({fpmas::communication::WORLD.getRank(), int_dist(gen)});

	// Randomly choses 50000 items on each process from all processes
	auto local_choices = fpmas::random::split_choices(
			fpmas::communication::WORLD, local_items, N_CHOICES);

	for(auto item : local_choices)
		// Ensures that only local items are selected
		ASSERT_THAT(item.first, fpmas::communication::WORLD.getRank());

	fpmas::communication::TypedMpi<std::vector<std::pair<int, int>>> item_mpi(
			fpmas::communication::WORLD
			);
	auto global_choices = item_mpi.allGather(local_choices);
	auto choices = std::accumulate(
			global_choices.begin(), global_choices.end(),
			std::vector<std::pair<int, int>>(), fpmas::utils::Concat()
			);
	ASSERT_THAT(choices, SizeIs(N_CHOICES));

	std::vector<int> process_histogram(fpmas::communication::WORLD.getSize());
	std::vector<std::vector<int>> value_histograms(fpmas::communication::WORLD.getSize());
	for(int i = 0; i < fpmas::communication::WORLD.getSize(); i++)
		value_histograms[i].resize(100);

	for(auto item : choices) {
		process_histogram[item.first]++;
		value_histograms[item.first][item.second]++;
	}

	// Ensures that values are uniformly selected from all processes
	for(auto item : process_histogram)
		ASSERT_NEAR((float) item / N_CHOICES, 1.f / fpmas::communication::WORLD.getSize(), 0.1);

	// Ensures that values are uniformly selected on each process
	for(int rank = 0; rank < fpmas::communication::WORLD.getSize(); rank++) {
		for(auto item : value_histograms[rank])
			ASSERT_NEAR((float) item / global_choices[rank].size(), 1.f/100, 0.1f);
	}
}

TEST(Random, local_sample_sequential_gen) {
	static const std::size_t N_LOCAL_ITEMS = 100;
	static const std::size_t N_SAMPLES = 50;
	static const std::size_t N_ROUNDS = 10000;

	fpmas::random::mt19937_64 gen;
	std::vector<int> local_items(N_LOCAL_ITEMS);
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items[i] = i;

	fpmas::communication::TypedMpi<std::vector<int>> vector_id_mpi(fpmas::communication::WORLD);

	// Counts how many times each item has been selected
	std::vector<std::size_t> histogram(N_LOCAL_ITEMS);
	for(std::size_t n = 0; n < N_ROUNDS; n++) {
		std::vector<int> sample = fpmas::random::local_sample(local_items, N_SAMPLES, gen);
		// Ensures each item in sample is unique
		std::set<int> sample_set;
		for(auto item : sample)
			sample_set.insert(item);
		ASSERT_EQ(sample_set.size(), sample.size());
		for(auto item : sample_set)
			histogram[item]++;

		// Ensures that the same sample is generated on all processes, since
		// the provided random generator is sequential
		std::vector<std::vector<int>> global_samples
			= vector_id_mpi.gather(sample, 0);

		FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
			for(std::size_t i = 1; i < global_samples.size(); i++)
				ASSERT_THAT(global_samples[i], ElementsAreArray(global_samples[0]));
		}
		fpmas::communication::WORLD.barrier();
	}
	for(std::size_t i = 0; i < histogram.size(); i++) {
		// Ensures that the chance to take each item at each round is k/n
		ASSERT_NEAR((float) histogram[i] / N_ROUNDS, (float) N_SAMPLES / N_LOCAL_ITEMS, 0.1);
	}
}

TEST(Random, local_sample_distributed_gen) {
	static const std::size_t N_LOCAL_ITEMS = 100;
	static const std::size_t N_SAMPLES = 50;
	static const std::size_t N_ROUNDS = 10000;

	std::vector<int> local_items(N_LOCAL_ITEMS);
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items[i] = i;


	// Counts how many times each item has been selected
	std::vector<std::size_t> histogram(N_LOCAL_ITEMS);
	for(std::size_t n = 0; n < N_ROUNDS; n++) {
		std::vector<int> sample = fpmas::random::local_sample(local_items, N_SAMPLES);
		// Ensures each item in sample is unique
		std::set<int> sample_set;
		for(auto item : sample)
			sample_set.insert(item);
		ASSERT_EQ(sample_set.size(), sample.size());
		for(auto item : sample_set)
			histogram[item]++;

		// Ensures that different samples are generated on all processes, since
		// the provided random generator is distributed
		fpmas::communication::TypedMpi<std::vector<int>> vector_id_mpi(fpmas::communication::WORLD);
		std::vector<std::vector<int>> global_samples
			= vector_id_mpi.gather(sample, 0);

		FPMAS_ON_PROC(fpmas::communication::WORLD, 0) {
			for(std::size_t i = 1; i < global_samples.size(); i++)
				for(std::size_t j = 0; j < i; j++)
					ASSERT_THAT(global_samples[i], Not(ElementsAreArray(global_samples[j])));
		}
		fpmas::communication::WORLD.barrier();
	}
	for(std::size_t i = 0; i < histogram.size(); i++) {
		// Ensures that the chance to take each item at each round is k/n
		ASSERT_NEAR((float) histogram[i] / N_ROUNDS, (float) N_SAMPLES / N_LOCAL_ITEMS, 0.1);
	}
}

struct index_cmp {
	bool operator()(const std::pair<int, int>& i1, const std::pair<int, int>& i2) const {
		if(i1.first == i2.first)
			return i1.second < i2.second;
		return i1.first < i2.first;
	}
};


TEST(Random, distributed_sample) {
	static const std::size_t N_LOCAL_ITEMS = 100000;
	static const std::size_t N_CHOICES = N_LOCAL_ITEMS;

	// Local items are pair of values such as `first=current rank` and
	// `second=uniformly selected integer in [0, 100]`
	std::vector<std::pair<int, int>> local_items;
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items.push_back({fpmas::communication::WORLD.getRank(), i});

	// Randomly choses 50000 items on each process from all processes
	auto gather_sample = fpmas::random::distributed_sample(
			fpmas::communication::WORLD, local_items, N_CHOICES);

	// Accumulates results in a single vector
	std::vector<std::pair<int, int>> samples = std::accumulate(
			gather_sample.begin(), gather_sample.end(), std::vector<std::pair<int, int>>(),
			fpmas::utils::Concat());


	// Ensures all items are unique
	std::set<std::pair<int, int>, index_cmp> sample_set;
	for(auto item : samples)
		sample_set.insert(item);
	ASSERT_THAT(sample_set, SizeIs(N_CHOICES));

	// Ensures item are selected from all processes
	std::vector<std::size_t> histogram(fpmas::communication::WORLD.getSize());
	for(auto item : sample_set)
		histogram[item.first]++;
	for(auto item : histogram) {
		ASSERT_NEAR(
				(float) item / N_CHOICES,
				1.f / fpmas::communication::WORLD.getSize(),
				0.1
				);
	}
}

TEST(Random, distributed_sample_n_sup_size) {
	static const std::size_t N_LOCAL_ITEMS = 100000;

	std::size_t total_item_count
		= N_LOCAL_ITEMS * fpmas::communication::WORLD.getSize();

	// Local items are pair of values such as `first=current rank` and
	// `second=uniformly selected integer in [0, 100]`
	std::vector<std::pair<int, int>> local_items;
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items.push_back({fpmas::communication::WORLD.getRank(), i});

	// Randomly choses 50000 items on each process from all processes
	auto gather_sample = fpmas::random::distributed_sample(
			fpmas::communication::WORLD, local_items, total_item_count+10);

	// Accumulates results in a single vector
	std::vector<std::pair<int, int>> samples = std::accumulate(
			gather_sample.begin(), gather_sample.end(), std::vector<std::pair<int, int>>(),
			fpmas::utils::Concat());


	// Ensures all items are unique
	std::set<std::pair<int, int>, index_cmp> sample_set;
	for(auto item : samples)
		sample_set.insert(item);
	// Only total_item_count can be selected (all items)
	ASSERT_THAT(sample_set, SizeIs(total_item_count));

	std::set<std::pair<int, int>, index_cmp> expected_total_items;
	for(int p = 0; p < fpmas::communication::WORLD.getSize(); p++)
		for(int i = 0; i < (int) N_LOCAL_ITEMS; i++)
			expected_total_items.insert({p, i});

	ASSERT_THAT(sample_set, ElementsAreArray(expected_total_items));
}

TEST(Random, sample_indexes) {
	static const std::size_t SAMPLE_SIZE = 20;
	static const std::size_t NUM_VALUES = 100;
	static const std::size_t NUM_ROUNDS = 1000;

	std::vector<std::size_t> histogram(NUM_VALUES);
	fpmas::random::mt19937_64 gen;
	for(std::size_t i = 0; i < NUM_ROUNDS; i++) {
		std::vector<std::size_t> sample = fpmas::random::sample_indexes(
				0ul, NUM_VALUES, SAMPLE_SIZE, gen);
		ASSERT_THAT(sample, SizeIs(SAMPLE_SIZE));

		std::set<std::size_t> sample_set(sample.begin(), sample.end());

		ASSERT_THAT(sample_set, SizeIs(SAMPLE_SIZE));
		for(auto item : sample)
			histogram[item]++;
	}
	for(auto item : histogram)
		ASSERT_NEAR((float) item / NUM_ROUNDS, (float) SAMPLE_SIZE / NUM_VALUES, 0.1); 
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

TEST(Random, sample_distributed_indexes) {
	using fpmas::random::DistributedIndex;

	static const std::size_t N_LOCAL_ITEMS = 20;
	static const std::size_t N_CHOICES = N_LOCAL_ITEMS;
	static const std::size_t NUM_ROUNDS = 1000;

	fpmas::random::mt19937_64 gen;
	fpmas::random::UniformIntDistribution<int> rd_count(-9, 10);
	std::map<int, std::size_t> num_items;
	std::size_t n_items = 0;
	for(int i = 0; i < fpmas::communication::WORLD.getSize(); i++) {
		num_items[i] = N_LOCAL_ITEMS+rd_count(gen);
		n_items+=num_items[i];
	}

	std::vector<std::size_t> histogram(n_items);
	for(std::size_t i = 0; i < NUM_ROUNDS; i++) {
		// Randomly choses 50000 items on each process from all processes
		auto local_sample = fpmas::random::sample_indexes(
				DistributedIndex::begin(&num_items),
				DistributedIndex::end(&num_items),
				N_CHOICES, gen);

		ASSERT_THAT(local_sample, SizeIs(N_CHOICES));


		// Ensures all items are unique
		std::set<DistributedIndex, dist_index_cmp> sample_set(
				local_sample.begin(), local_sample.end()
				);
		ASSERT_THAT(sample_set, SizeIs(N_CHOICES));
		for(auto item : sample_set) {
			// Ensures that selected items are valid
			ASSERT_THAT(item.key(), AllOf(Ge(0), Lt(fpmas::communication::WORLD.getSize())));
			ASSERT_THAT(item.offset(), AllOf(Ge(0), Lt(num_items[item.key()])));
		}

		// Ensures item are selected from all processes
		const DistributedIndex begin = DistributedIndex::begin(&num_items);
		for(auto item : sample_set)
			histogram[DistributedIndex::distance(begin, item)]++;
	}
	for(auto item : histogram)
		ASSERT_NEAR((float) item / NUM_ROUNDS, (float) N_CHOICES / n_items, 0.1);
}

TEST(Random, split_sample) {
	static const std::size_t N_LOCAL_ITEMS = 100000;
	static const std::size_t N_CHOICES = N_LOCAL_ITEMS;

	// Local items are pair of values such as `first=current rank` and
	// `second=uniformly selected integer in [0, 100]`
	std::vector<std::pair<int, int>> local_items;
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items.push_back({fpmas::communication::WORLD.getRank(), i});

	// Randomly choses 50000 items on each process from all processes
	auto local_sample = fpmas::random::split_sample(
			fpmas::communication::WORLD, local_items, N_CHOICES);

	for(auto item : local_sample)
		// Ensures only local items are selected
		ASSERT_EQ(item.first, fpmas::communication::WORLD.getRank());

	fpmas::communication::TypedMpi<std::vector<std::pair<int, int>>> mpi(
			fpmas::communication::WORLD
			);
	std::vector<std::vector<std::pair<int, int>>> global_samples
		= mpi.allGather(local_sample);

	// Accumulates results in a single vector
	std::vector<std::pair<int, int>> samples = std::accumulate(
			global_samples.begin(), global_samples.end(),
			std::vector<std::pair<int, int>>(),
			fpmas::utils::Concat());
	ASSERT_THAT(samples, SizeIs(N_CHOICES));


	// Ensures all items are unique
	std::set<std::pair<int, int>, index_cmp> sample_set;
	for(auto item : samples)
		sample_set.insert(item);
	ASSERT_THAT(sample_set, SizeIs(N_CHOICES));

	// Ensures item are selected from all processes
	std::vector<std::size_t> histogram(fpmas::communication::WORLD.getSize());
	for(auto item : sample_set)
		histogram[item.first]++;
	for(auto item : histogram) {
		ASSERT_NEAR(
				(float) item / N_CHOICES,
				1.f / fpmas::communication::WORLD.getSize(),
				0.1
				);
	}
}

TEST(Random, split_sample_n_sup_size) {
	static const std::size_t N_LOCAL_ITEMS = 1000;

	std::size_t total_item_count =
		N_LOCAL_ITEMS * fpmas::communication::WORLD.getSize();

	// Local items are pair of values such as `first=current rank` and
	// `second=uniformly selected integer in [0, 100]`
	std::vector<std::pair<int, int>> local_items;
	for(std::size_t i = 0; i < N_LOCAL_ITEMS; i++)
		local_items.push_back({fpmas::communication::WORLD.getRank(), i});

	// Randomly choses 50000 items on each process from all processes
	auto local_sample = fpmas::random::split_sample(
			fpmas::communication::WORLD, local_items, total_item_count+10);

	ASSERT_THAT(local_sample, UnorderedElementsAreArray(local_items));
}
