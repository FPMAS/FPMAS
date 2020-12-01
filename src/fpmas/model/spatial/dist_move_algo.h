#ifndef FPMAS_DIST_MOVE_ALGO_H
#define FPMAS_DIST_MOVE_ALGO_H

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
					StaticEndCondition() {
						static const RangeType range(MaxRangeSize);
						max_step = range.radius(nullptr);
					}

					void init(
							api::communication::MpiCommunicator&,
							std::vector<api::model::SpatialAgent<CellType>*>,
							std::vector<CellType*>) override {
						_step = 0;
					}
					void step() override {
						_step++;
					}
					bool end() override {
						return _step == max_step;
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
					return _step == max_step;
				}
		};


		template<typename CellType, typename EndCondition>
			class DistributedMoveAlgorithm : public api::model::DistributedMoveAlgorithm<CellType> {
				private:
					class AlgoTask : public api::scheduler::Task {
						private:
							std::vector<api::model::SpatialAgent<CellType>*> agents;
							std::vector<CellType*> cells;
							EndCondition end;
							Behavior<api::model::CellBehavior> cell_behaviors {
								&api::model::CellBehavior::handleNewLocation,
									&api::model::CellBehavior::handleMove,
									&api::model::CellBehavior::handlePerceive
							};
							Behavior<api::model::SpatialAgentBehavior> spatial_agent_behaviors {
								&api::model::SpatialAgentBehavior::handleNewMove,
									&api::model::SpatialAgentBehavior::handleNewPerceive
							};
							Behavior<api::model::CellBehavior> update_perceptions_behavior {
								&api::model::CellBehavior::updatePerceptions
							};
							api::model::AgentGraph* graph;

						public:
							void set(
									api::model::AgentGraph* graph,
									std::vector<api::model::SpatialAgent<CellType>*> agents,
									std::vector<CellType*> cells) {
								this->graph = graph;
								this->agents = agents;
								this->cells = cells;
							}

							void run() override {
								end.init(graph->getMpiCommunicator(), agents, cells);

								while(!end.end()) {
									for(auto cell : cells)
										cell_behaviors.execute(cell);
									graph->synchronize();
									for(auto agent : agents)
										spatial_agent_behaviors.execute(agent);
									graph->synchronize();
									end.step();
								}
								for(auto cell : cells)
									update_perceptions_behavior.execute(cell);
								graph->synchronize();
							}
					};

					AlgoTask algo_task;
					scheduler::Job algo_job;


				public:
					DistributedMoveAlgorithm() {
						algo_job.add(algo_task);
					}

					api::scheduler::JobList jobs(
							api::model::SpatialModel<CellType>& model,
							std::vector<api::model::SpatialAgent<CellType>*> agents,
							std::vector<CellType*> cells
							) override {
						algo_task.set(&model.graph(), agents, cells);
						api::scheduler::JobList _jobs;

						_jobs.push_back(algo_job);
						return _jobs;

					}
			};

	}
}
#endif
