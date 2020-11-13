#include "distributed_id.h"

namespace fpmas {
	std::string to_string(const api::graph::DistributedId& id) {
		return "[" + std::to_string(id.rank()) + ":" + std::to_string(id.id()) + "]";
	}
}

namespace fpmas { namespace api { namespace graph {
	std::ostream& operator<<(std::ostream& os, const DistributedId& id) {
		os << fpmas::to_string(id);
		return os;
	}

	MPI_Datatype DistributedId::mpiDistributedIdType;
}}}


