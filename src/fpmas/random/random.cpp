#include "random.h"

namespace fpmas { namespace random {
	DistributedGenerator<> rd_choices;
	const std::mt19937::result_type default_seed {std::mt19937::default_seed};
	mt19937::result_type seed {default_seed};

}}
