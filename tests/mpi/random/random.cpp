#include "fpmas/random/random.h"
#include "fpmas/utils/functional.h"
#include "fpmas/utils/macros.h"
#include <algorithm>

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
