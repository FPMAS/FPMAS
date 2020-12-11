#ifndef FPMAS_DIST_MOVE_ALGO_H
#define FPMAS_DIST_MOVE_ALGO_H

/**\file src/fpmas/model/spatial/dist_move_algo.h
 * Distribued move algorithm implementation.
 */

#include "fpmas/api/model/spatial/spatial_model.h"
#include "../model.h"

namespace fpmas {
	namespace model {
		/**
		 * Implements a static api::model::EndCondition for the
		 * DistributedMoveAlgorithm, based on the provided range and the maximum
		 * range size.
		 *
		 * `RangeType` requirements:
		 * - constructible from the `MaxRangeSize` parameter
		 * - given `const RangeType range(MaxRangeSize)`,
		 *   `range.radius(nullptr)` must return a valid radius. This radius is
		 *   considered as the "maximum radius of any range of type RangeType in
		 *   the simulation". `nullptr` is passed as the `origin` argument to
		 *   `radius()`, since the **static** value returned must be independent
		 *   from the range's origin.
		 *
		 * A **static** required number of steps compatible with the
		 * DistributedMoveAlgorithm is then determined from the computed maximum
		 * radius. Ranges are still allowed to evolve dynamically at runtime, as
		 * long as their radius stays less than or equal to the maximum radius.
		 */
		template<typename RangeType, unsigned int MaxRangeSize, typename CellType>
			class StaticEndCondition : public api::model::EndCondition<CellType> {
				private:
					std::size_t _step;
					std::size_t max_step;

				public:
					/**
					 * StaticEndCondition constructor.
					 *
					 * The maximum iteration count is computed from the
					 * `RangeType` radius() and the `MaxRangeSize`.
					 */
					StaticEndCondition() {
						static const RangeType range(MaxRangeSize);
						max_step = range.radius(nullptr);
					}

					/**
					 * Initializes the end condition.
					 */
					void init(
							api::communication::MpiCommunicator&,
							std::vector<api::model::SpatialAgent<CellType>*>,
							std::vector<CellType*>) override {
						_step = 0;
					}

					/**
					 * Increments internal counter.
					 */
					void step() override {
						_step++;
					}

					/**
					 * Returns true if and only if the internal counter is
					 * greater or equal to the maximum iteration count.
					 *
					 * @return true iff the end condition has been reached
					 */
					bool end() override {
						return _step >= max_step;
					}
			};

		/**
		 * Implements a dynamic api::model::EndCondition for the
		 * DistributedMoveAlgorithm.
		 *
		 * The required number of steps that must be performed by the
		 * DistributedMoveAlgorithm is determined at runtime, at each
		 * initialization of the end condition. To do so, an algorithm
		 * determines the current maximum radius of mobility and perception
		 * ranges of **all** SpatialAgents in the simulation (more
		 * particularly, those provided to the init() method). In consequence,
		 * inter-processes communications occur to globally determine the
		 * maximum.
		 *
		 * This EndCondition might be less efficient than the
		 * StaticEndCondition. However, it allows mobility and perception
		 * ranges of each SpatialAgent to evolve dynamically at runtime,
		 * without any restriction. Moreover, the computed number of steps is
		 * always optimized, and exactly corresponds to the minimal number of
		 * steps required by the DistributedMoveAlgorithm to properly update
		 * SpatialAgent locations.
		 */
		template<typename CellType>
		class DynamicEndCondition : public api::model::EndCondition<CellType> {
			private:
				std::size_t _step;
				std::size_t max_step;
			public:
				/**
				 * Initializes the end condition, computing the requied number
				 * of steps from the current perception and mobility ranges of
				 * all `agents` on all processes.
				 *
				 * @param comm MPI communicator
				 * @param agents moving agents
				 */
				void init(
						api::communication::MpiCommunicator& comm,
						std::vector<api::model::SpatialAgent<CellType>*> agents,
						std::vector<CellType*>) override {
					std::size_t local_max = 0;
					for(auto agent : agents) {
						std::size_t radius = std::max(
								agent->mobilityRange().radius(agent->locationCell()),
								agent->perceptionRange().radius(agent->locationCell())
								);
						if(radius > local_max)
							local_max = radius;
					}
					communication::TypedMpi<std::size_t> mpi(comm);
					std::vector<std::size_t> local_max_list = mpi.allGather(local_max);
					this->max_step = *std::max_element(local_max_list.begin(), local_max_list.end());

					_step = 0;
				}

				void step() override {
					_step++;
				}
				bool end() override {
					return _step >= max_step;
				}
		};


		/**
		 * api::model::DistributedMoveAlgorithm implementation.
		 */
		template<typename CellType>
			class DistributedMoveAlgorithm : public api::model::DistributedMoveAlgorithm<CellType> {
				private:
					class AlgoTask : public api::scheduler::Task {
						private:
							typedef api::model::CellBehavior CellBehavior;
							typedef api::model::SpatialAgentBehavior AgentBehavior;

							DistributedMoveAlgorithm& dist_move_algo;

							Behavior<CellBehavior> cell_behaviors {
								&CellBehavior::handleNewLocation,
								&CellBehavior::handleMove,
								&CellBehavior::handlePerceive
							};
							Behavior<AgentBehavior> spatial_agent_behaviors {
								&AgentBehavior::handleNewMove,
								&AgentBehavior::handleNewPerceive
							};
							Behavior<CellBehavior> update_perceptions_behavior {
								&CellBehavior::updatePerceptions
							};

						public:
							AlgoTask(DistributedMoveAlgorithm& dist_move_algo)
								: dist_move_algo(dist_move_algo) {}
							
							/**
							 * Runs the distributed move algorithm
							 */
							void run() override;
					};

					api::model::SpatialModel<CellType>& model;
					api::model::AgentGroup& move_agent_group;
					api::model::AgentGroup& cell_group;
					api::model::EndCondition<CellType>& end;

					AlgoTask algo_task;
					scheduler::Job algo_job;

				public:
					/**
					 * DistributedMoveAlgorithm constructor.
					 */
					DistributedMoveAlgorithm(
							api::model::SpatialModel<CellType>& model,
							api::model::AgentGroup& move_agent_group,
							api::model::AgentGroup& cell_group,
							api::model::EndCondition<CellType>& end
							) :
						model(model), move_agent_group(move_agent_group),
						cell_group(cell_group), end(end), algo_task(*this) {
						algo_job.add(algo_task);
					}

					/**
					 * \copydoc api::model::DistributedMoveAlgorithm::jobs
					 */
					api::scheduler::JobList jobs() const override {
						return {algo_job};
					}
			};

		template<typename CellType>
			void DistributedMoveAlgorithm<CellType>::AlgoTask::run() {
				FPMAS_LOGD(this->dist_move_algo.model.getMpiCommunicator().getRank(),
						"DMA", "Running DistributedMoveAlgorithm...", "");
				using api::model::SpatialAgent;

				std::vector<SpatialAgent<CellType>*> agents;
				for(auto agent : dist_move_algo.move_agent_group.localAgents()) {
					agents.push_back(
							dynamic_cast<SpatialAgent<CellType>*>(agent));
				}
				std::vector<CellType*> cells;
				for(auto cell : dist_move_algo.cell_group.localAgents()) {
					cells.push_back(dynamic_cast<CellType*>(cell));
				}

				// Initializes the generic end condition
				dist_move_algo.end.init(dist_move_algo.model.getMpiCommunicator(), agents, cells);

				// Assumed to loop until the mobility and
				// perception fields of all agents are up to
				// date
				while(!dist_move_algo.end.end()) {
					for(auto cell : cells)
						// Grows mobility and perception fields
						cell_behaviors.execute(cell);
					dist_move_algo.model.graph().synchronize();
					for(auto agent : agents)
						// Crops and build mobility and
						// perception fields
						spatial_agent_behaviors.execute(agent);
					dist_move_algo.model.graph().synchronize();
					dist_move_algo.end.step();
				}
				for(auto cell : cells)
					// Update agent perceptions (creates
					// PERCEPTION links)
					update_perceptions_behavior.execute(cell);
				dist_move_algo.model.graph().synchronize();

				FPMAS_LOGD(this->dist_move_algo.model.getMpiCommunicator().getRank(),
						"DMA", "Done.", "");
			}
	}
}
#endif
