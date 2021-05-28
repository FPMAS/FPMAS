#include "distributed_id.h"

namespace fpmas {
	std::string to_string(const api::graph::DistributedId& id) {
		return "[" + std::to_string(id.rank()) + ":" + std::to_string(id.id()) + "]";
	}

	int DistributedId::max_rank = std::numeric_limits<int>::max();
	FPMAS_ID_TYPE DistributedId::max_id = std::numeric_limits<FPMAS_ID_TYPE>::max();
}

namespace fpmas { namespace api { namespace graph {
	std::ostream& operator<<(std::ostream& os, const DistributedId& id) {
		os << fpmas::to_string(id);
		return os;
	}

	MPI_Datatype DistributedId::mpiDistributedIdType;
}}}


