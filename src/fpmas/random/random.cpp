#include "random.h"

namespace fpmas { namespace random {
	DistributedGenerator<> rd_choices;
	const std::mt19937::result_type default_seed {std::mt19937::default_seed};
	mt19937::result_type seed {default_seed};

	DistributedIndex DistributedIndex::begin(const std::vector<std::size_t>& item_counts) { 
		int p = 0;
		while(item_counts[p] == 0)
			++p;
		return DistributedIndex(item_counts, p, 0);
	}

	DistributedIndex DistributedIndex::end(const std::vector<std::size_t>& item_counts) {
		return DistributedIndex(item_counts, item_counts.size(), 0);
	}

	DistributedIndex& DistributedIndex::operator++() {
		if(p != (int) item_counts.size()) {
			if(offset >= item_counts[p]-1) {
				offset=0;
				++p;
				// Skips processes without items
				while(item_counts[p] == 0)
					++p;
			} else {
				++offset;
			}
		}
		return *this;
	}

	DistributedIndex DistributedIndex::operator++(int) {
		DistributedIndex index = *this;
		++*this;
		return index;
	}

	DistributedIndex DistributedIndex::operator+(std::size_t n) const {
		DistributedIndex index = *this;
		if(index.offset+n < item_counts[index.p]) {
			// Stay in current process, nothing to do
			index.offset+=n;
		} else {
			// Need to go to next process
			n -= item_counts[p]-index.offset;
			index.offset = 0;
			++index.p;
			while(n >= item_counts[index.p]) {
				n-= item_counts[index.p];
				++index.p;
			}
			index.offset = n;
		}
		return index;
	}
	
	std::size_t DistributedIndex::distance(
			const DistributedIndex& i1, const DistributedIndex& i2) {
		if(i1.p == i2.p) {
			return i2.offset - i1.offset;
		} else {
			std::size_t result = i1.item_counts[i1.p] - i1.offset;
			int p = i1.p;
			++p;
			while(p != i2.p) {
				result+=i1.item_counts[p];
				++p;
			}
			result+=i2.offset;
			return result;
		}
	}

	bool DistributedIndex::operator==(const DistributedIndex& index) const {
		return p == index.p && offset == index.offset;
	}

	bool DistributedIndex::operator!=(const DistributedIndex& index) const {
		return p!=index.p || offset != index.offset;
	}


}}
