#ifndef FPMAS_MODEL_EXCEPTIONS_API_H
#define FPMAS_MODEL_EXCEPTIONS_API_H

/** \file src/fpmas/api/model/exceptions.h
 * Model related exceptions.
 */

#include "fpmas/api/graph/distributed_id.h"
#include "fpmas/api/model/spatial/grid.h"

namespace fpmas { namespace api { namespace model {

	/**
	 * Exception thrown when attempting to access something outside of an
	 * Agent's perception or mobility field.
	 */
	class OutOfFieldException : public std::domain_error {
		public:
			/**
			 * OutOfFieldException constructor.
			 *
			 * @param what message
			 */
			OutOfFieldException(const char* what)
				: std::domain_error(what) {}
	};

	/**
	 * Exception that might be thrown when attempting to move to a Cell outside
	 * of the SpatialAgent's mobility field.
	 */
	class OutOfMobilityFieldException : public OutOfFieldException {
		public:
			/**
			 * OutOfMobilityFieldException constructor.
			 *
			 * @param agent_id id of the agent to move
			 * @param cell_id id that does not correspond to any Cell in the
			 * agent's mobility field
			 */
			OutOfMobilityFieldException(
					fpmas::api::graph::DistributedId agent_id,
					fpmas::api::graph::DistributedId cell_id)
				: OutOfFieldException((
						std::string("Cell ") + fpmas::to_string(cell_id)
						+ " is out of the mobility range of Agent "
						+ fpmas::to_string(agent_id)).c_str()) {}

			/**
			 * OutOfMobilityFieldException constructor adapted to \GridAgents.
			 *
			 * @param agent_id id of the agent to move
			 * @param agent_location location of the agent
			 * @param cell_point point that does not correspond to any GridCell in
			 * the agent's mobility field
			 */
			OutOfMobilityFieldException(
					fpmas::api::graph::DistributedId agent_id,
					fpmas::api::model::DiscretePoint agent_location,
					fpmas::api::model::DiscretePoint cell_point)
				: OutOfFieldException((
						std::string("Point ") + fpmas::to_string(cell_point)
						+ " is out of the mobility range of Agent "
						+ fpmas::to_string(agent_id) + ", currently located on "
						+ fpmas::to_string(agent_location)).c_str()) {}
	};
}}}
#endif
