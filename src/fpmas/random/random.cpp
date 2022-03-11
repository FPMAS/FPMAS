#include "random.h"

namespace fpmas { namespace random {
	DistributedGenerator<> rd_choices;
	const std::mt19937::result_type default_seed {std::mt19937::default_seed};
	mt19937::result_type seed {default_seed};

	std::map<int, std::size_t> DistributedIndex::vec_to_map(
			const std::vector<std::size_t>& vec) {
		std::map<int, std::size_t> map;
		for(std::size_t i = 0; i < vec.size(); i++)
			map.insert(map.end(), {i, vec[i]});
		return map;
	}

	DistributedIndex DistributedIndex::begin(const std::vector<std::size_t>& item_counts) {
		return Index<int>::begin(vec_to_map(item_counts));
	}

	DistributedIndex DistributedIndex::end(const std::vector<std::size_t>& item_counts) {
		return Index<int>::end(vec_to_map(item_counts));
	}
}}
