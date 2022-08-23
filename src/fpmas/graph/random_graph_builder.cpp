#include "random_graph_builder.h"

namespace fpmas { namespace graph {
	random::DistributedGenerator<> RandomGraphBuilder::distributed_rd;
	random::mt19937_64 RandomGraphBuilder::rd;

	void RandomGraphBuilder::seed(random::mt19937_64::result_type seed) {
		distributed_rd = {seed};
		rd = {seed};
	}
}}
