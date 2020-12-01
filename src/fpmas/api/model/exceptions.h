#ifndef FPMAS_MODEL_EXCEPTIONS_API_H
#define FPMAS_MODEL_EXCEPTIONS_API_H

#include "fpmas/api/graph/distributed_id.h"
#include "fpmas/api/model/spatial/grid.h"

namespace fpmas { namespace api { namespace model {

	class OutOfFieldException : public std::domain_error {
		public:
			OutOfFieldException(const char* what)
				: std::domain_error(what) {}
	};

	class OutOfMobilityFieldException : public OutOfFieldException {
		public:
			OutOfMobilityFieldException(
					fpmas::api::graph::DistributedId agent_id,
					fpmas::api::graph::DistributedId cell_id)
				: OutOfFieldException((
						std::string("Cell ") + fpmas::to_string(cell_id)
						+ " is out of the mobility range of Agent "
						+ fpmas::to_string(agent_id)).c_str()) {}

			OutOfMobilityFieldException(
					fpmas::api::graph::DistributedId agent_id,
					fpmas::api::model::DiscretePoint agent_location,
					fpmas::api::model::DiscretePoint point)
				: OutOfFieldException((
						std::string("Point ") + fpmas::to_string(point)
						+ " is out of the mobility range of Agent "
						+ fpmas::to_string(agent_id) + ", currently located on "
						+ fpmas::to_string(agent_location)).c_str()) {}
	};
}}}
#endif
