#ifndef FPMAS_SPATIAL_AGENT_MAPPING_H
#define FPMAS_SPATIAL_AGENT_MAPPING_H

/** \file src/fpmas/model/spatial/spatial_agent_mapping.h
 * Features used to map agents on generic spatial graphs.
 */

#include "fpmas/api/model/spatial/spatial_model.h"
#include "fpmas/random/random.h"

namespace fpmas { namespace model {

	/**
	 * Helper class that defines a static random::mt19937_64 generator that
	 * must be used by api::model::SpatialAgentMapping implementations in order
	 * to be seeded by the fpmas::seed() method.
	 *
	 * A non-distributed random generator is used, in order to generate the
	 * same random sequence on all processes, what is consistent with the
	 * api::model::SpatialAgentMapping specification that states that the same
	 * mapping must be generated on all processes.
	 *
	 * @see UniformAgentMapping
	 * @see UniformGridAgentMapping
	 * @see ConstrainedGridAgentMapping
	 */
	struct RandomMapping {
		/**
		 * Static random generator.
		 */
		static random::mt19937_64 rd;

		/**
		 * Seeds the random generator.
		 *
		 * @param seed random seed
		 */
		static void seed(random::mt19937_64::result_type seed);
	};

	/**
	 * An api::model::SpatialAgentMapping implementation that randomly assign
	 * \SpatialAgents to available \Cells.
	 *
	 * The RandomMapping::rd random generator is used, and can be seeded with
	 * RandomMapping::seed() or fpmas::seed().
	 */
	class UniformAgentMapping : public api::model::SpatialAgentMapping<api::model::Cell> {
		private:
			std::unordered_map<DistributedId, std::size_t> mapping;

		public:
			/**
			 * UniformAgentMapping constructor.
			 *
			 * `agent_count` agents are distributed on the distributed set of
			 * \Cells contained in the specified `cell_group`.
			 *
			 * For the same reasons, the same `agent_count` must be specified
			 * on all processes, since this corresponds to the **global** count
			 * of \SpatialAgents to initialize.
			 *
			 * This constructor call is **synchronous**: since global MPI
			 * communications are performed, all processes block until the
			 * constructor is called on all processes.
			 *
			 * @param comm MPI communicator
			 * @param cell_group AgentGroup containing \Cells to which a
			 * \SpatialAgent count will be assigned.
			 * @param agent_count total number of \SpatialAgents to distribute
			 * on a set of \Cells. 
			 */
			UniformAgentMapping(
					api::communication::MpiCommunicator& comm,
					api::model::AgentGroup& cell_group,
					std::size_t agent_count
					);

			std::size_t countAt(api::model::Cell* cell) override;
	};
}}
#endif
