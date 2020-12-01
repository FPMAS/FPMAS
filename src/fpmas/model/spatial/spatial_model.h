#ifndef FPMAS_ENVIRONMENT_H
#define FPMAS_ENVIRONMENT_H

#include "dist_move_algo.h"
#include "fpmas/api/model/exceptions.h"
#include "../serializer.h"

namespace fpmas {
	namespace model {
	using api::model::SpatialModelLayers;

	template<typename CellType>
	class MoveAgentGroup : public model::detail::AgentGroupBase {
		private:
			api::model::SpatialModel<CellType>& model;
		public:
			MoveAgentGroup(
					api::model::GroupId group_id,
					api::model::AgentGraph& agent_graph,
					api::model::SpatialModel<CellType>& model) :
			AgentGroupBase(group_id, agent_graph), model(model) {
			}

			MoveAgentGroup(
					api::model::GroupId group_id,
					api::model::AgentGraph& agent_graph,
					const api::model::Behavior& behavior,
					api::model::SpatialModel<CellType>& model) :
			AgentGroupBase(group_id, agent_graph, behavior), model(model) {
			}
			api::scheduler::JobList initLocationJobs() const;
			api::scheduler::JobList jobs() const override;
	};
		
	template<typename CellType>
		api::scheduler::JobList MoveAgentGroup<CellType>::initLocationJobs() const {
			api::scheduler::JobList job_list;

			std::vector<api::model::SpatialAgent<CellType>*> agents;
			for(auto agent : this->localAgents())
				agents.push_back(dynamic_cast<api::model::SpatialAgent<CellType>*>(agent));

			for(auto job : model.distributedMoveAlgorithm().jobs(model, agents, model.cells()))
				job_list.push_back(job);
			return job_list;
		}

	template<typename CellType>
		api::scheduler::JobList MoveAgentGroup<CellType>::jobs() const {
			api::scheduler::JobList job_list;
			job_list.push_back(this->job());

			std::vector<api::model::SpatialAgent<CellType>*> agents;
			for(auto agent : this->localAgents()) {
				agents.push_back(dynamic_cast<api::model::SpatialAgent<CellType>*>(agent));
			}

			for(auto job : model.distributedMoveAlgorithm().jobs(model, agents, model.cells()))
				job_list.push_back(job);
			return job_list;
		}


	class CellBase : public virtual api::model::Cell, public NeighborsAccess {
		private:
			std::set<fpmas::api::graph::DistributedId> move_flags;
			std::set<fpmas::api::graph::DistributedId> perception_flags;

		protected:
			void updateLocation(api::model::Neighbor<api::model::Agent>& agent);
			void growMobilityRange(api::model::Agent* agent);
			void growPerceptionRange(api::model::Agent* agent);

		public:
			std::vector<api::model::Cell*> neighborhood() override;

			void handleNewLocation() override;
			void handleMove() override;
			void handlePerceive() override;

			void updatePerceptions() override;
	};

	template<typename CellType, typename TypeIdBase = CellType>
	class Cell : public CellBase, public AgentBase<CellType, TypeIdBase> {
	};

	static const api::model::GroupId CELL_GROUP_ID = -1;

	template<typename CellType, typename DistributedMoveAlgorithm>
	class SpatialModelBase : public api::model::SpatialModel<CellType> {
		private:
			AgentGroup* cell_group;
			DistributedMoveAlgorithm dist_move_algo;

		protected:
			void initCellGroup() {
				cell_group = &this->buildGroup(CELL_GROUP_ID);
			}

		public:
			void add(CellType* cell) override;
			std::vector<CellType*> cells() override;
			MoveAgentGroup<CellType>& buildMoveGroup(
					api::model::GroupId id, const api::model::Behavior& behavior) override;

			api::model::DistributedMoveAlgorithm<CellType>& distributedMoveAlgorithm() override;
	};

	template<typename CellType, typename DistMoveAlg>
		void SpatialModelBase<CellType, DistMoveAlg>::add(CellType* cell) {
			cell_group->add(cell);
		}

	template<typename CellType, typename DistMoveAlg>
		std::vector<CellType*> SpatialModelBase<CellType, DistMoveAlg>::cells() {
			std::vector<CellType*> cells;
			for(auto agent : cell_group->localAgents())
				cells.push_back(dynamic_cast<CellType*>(agent));
			return cells;
		}

	template<typename CellType, typename DistMoveAlg>
		MoveAgentGroup<CellType>& SpatialModelBase<CellType, DistMoveAlg>::buildMoveGroup(
				api::model::GroupId id, const api::model::Behavior& behavior) {
			auto* group = new MoveAgentGroup<CellType>(id, this->graph(), behavior, *this);
			this->insert(id, group);
			return *group;
		}

	template<typename CellType, typename DistMoveAlg>
		api::model::DistributedMoveAlgorithm<CellType>& SpatialModelBase<CellType, DistMoveAlg>::distributedMoveAlgorithm() {
			return dist_move_algo;
		}


	template<
		template<typename> class SyncMode,
		typename CellType,
		typename DistMoveAlgoEndCondition>
	class SpatialModel :
		public SpatialModelBase<CellType, DistributedMoveAlgorithm<CellType, DistMoveAlgoEndCondition>>,
		public Model<SyncMode> {
		public:
			SpatialModel() {
				this->initCellGroup();
			}
	};

	
	template<typename AgentType, typename CellType, typename Derived = AgentType>
	class SpatialAgentBase :
		public virtual api::model::SpatialAgent<CellType>,
		public model::AgentBase<AgentType, SpatialAgentBase<AgentType, CellType, Derived>> {
			friend nlohmann::adl_serializer<
				api::utils::PtrWrapper<SpatialAgentBase<AgentType, CellType, Derived>>>;

			public:
				typedef SpatialAgentBase<AgentType, CellType, Derived> JsonBase;

			private:
				class CurrentOutLayer {
					private:
						SpatialAgentBase* agent;
						fpmas::graph::LayerId layer_id;
						std::set<DistributedId> current_layer_ids;

					public:
						CurrentOutLayer(SpatialAgentBase* agent, fpmas::graph::LayerId layer_id)
							: agent(agent), layer_id(layer_id) {
								auto layer = agent->node()->outNeighbors(layer_id);
								for(auto node : layer)
									current_layer_ids.insert(node->getId());
							}

						bool contains(fpmas::api::model::Agent* agent) {
							return current_layer_ids.count(agent->node()->getId()) > 0;
						}

						void link(fpmas::api::model::Agent* cell) {
							current_layer_ids.insert(cell->node()->getId());
							agent->model()->link(agent, cell, layer_id);
						}
				};
			private:
				graph::DistributedId current_location_id;

				// Use pointers, since each instance (probably) belongs to the
				// implementing child class, or is static. In any case, it is
				// not the purpose of this base to perform copy assignment of
				// this fields, so a reference can't be used.
				const api::model::Range<CellType>* mobility_range;
				const api::model::Range<CellType>* perception_range;

			protected:
				SpatialAgentBase(
						const api::model::Range<CellType>& mobility_range,
						const api::model::Range<CellType>& perception_range) :
					mobility_range(&mobility_range),
					perception_range(&perception_range) {}

				void updateLocation(CellType* cell);

				CellType* locationCell() const override;
				void handleNewMove() override;
				void handleNewPerceive() override;

				using api::model::SpatialAgent<CellType>::moveTo;
				void moveTo(graph::DistributedId id) override;

			public:
				void initLocation(CellType* cell) override {
					this->moveTo(cell);
				}

				fpmas::graph::DistributedId locationId() const override {
					return current_location_id;
				}

				const api::model::Range<CellType>& mobilityRange() const override {
					return *mobility_range;
				}
				const api::model::Range<CellType>& perceptionRange() const override {
					return *perception_range;
				}
		};

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::updateLocation(CellType* cell) {
			// Links to new location
			this->model()->link(this, cell, SpatialModelLayers::NEW_LOCATION);

			// Unlinks obsolete location
			auto location = this->template outNeighbors<api::model::Cell>(
					SpatialModelLayers::LOCATION);
			if(location.count() > 0)
				this->model()->unlink(location.at(0).edge());

			// Unlinks obsolete mobility field
			for(auto cell : this->template outNeighbors<api::model::Cell>(
						SpatialModelLayers::MOVE)) {
				this->model()->unlink(cell.edge());
			}

			// Unlinks obsolete perception field
			for(auto agent : this->template outNeighbors<api::model::Agent>(
						SpatialModelLayers::PERCEPTION))
				this->model()->unlink(agent.edge());

			// Adds the NEW_LOCATION to the mobility/perceptions fields
			// depending on the current ranges
			if(this->mobility_range->contains(cell, cell))
				this->model()->link(this, cell, SpatialModelLayers::MOVE);
			if(this->perception_range->contains(cell, cell))
				this->model()->link(this, cell, SpatialModelLayers::PERCEIVE);

			this->current_location_id = cell->node()->getId();
		}

	template<typename AgentType, typename CellType, typename Derived>
		CellType* SpatialAgentBase<AgentType, CellType, Derived>::locationCell() const {
			auto location = this->template outNeighbors<CellType>(
					SpatialModelLayers::LOCATION);
			if(location.count() > 0)
				return location.at(0);
			return nullptr;
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewMove() {

			CurrentOutLayer move_layer(this, SpatialModelLayers::MOVE);

			for(auto cell : this->template outNeighbors<CellType>(
						SpatialModelLayers::NEW_MOVE)) {
				if(!move_layer.contains(cell)
						&& this->mobility_range->contains(this->locationCell(), cell))
					move_layer.link(cell);
				this->model()->unlink(cell.edge());
			}
		}

	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::handleNewPerceive() {
			CurrentOutLayer perceive_layer(this, SpatialModelLayers::PERCEIVE);

			for(auto cell : this->template outNeighbors<CellType>(
						SpatialModelLayers::NEW_PERCEIVE)) {
				if(!perceive_layer.contains(cell)
						&& this->perception_range->contains(this->locationCell(), cell))
					perceive_layer.link(cell);
				this->model()->unlink(cell.edge());
			}
		}
	
	template<typename AgentType, typename CellType, typename Derived>
		void SpatialAgentBase<AgentType, CellType, Derived>::moveTo(graph::DistributedId cell_id) {
			bool found = false;
			auto mobility_field = this->template outNeighbors<CellType>(SpatialModelLayers::MOVE);
			auto it = mobility_field.begin();
			while(!found && it != mobility_field.end()) {
				if((*it)->node()->getId() == cell_id) {
					found=true;
				} else {
					it++;
				}
			}
			if(found)
				this->moveTo(*it);
			else
				throw api::model::OutOfMobilityFieldException(this->node()->getId(), cell_id);
		}

	template<typename AgentType, typename CellType, typename Derived = AgentType>
		class SpatialAgent :
			public SpatialAgentBase<AgentType, CellType, Derived> {
				protected:
					using SpatialAgentBase<AgentType, CellType, Derived>::moveTo;
					using SpatialAgentBase<AgentType, CellType, Derived>::SpatialAgentBase;

					void moveTo(CellType* cell) override {
						this->updateLocation(cell);
					}
			};

	template<typename AgentType>
		class DefaultSpatialAgentFactory : public api::model::SpatialAgentFactory<typename AgentType::Cell> {
			public:
				AgentType* build() override {
					return new AgentType;
				}
		};

	template<typename CellType>
	class SpatialAgentBuilder : public api::model::SpatialAgentBuilder<CellType>{
		public:
			void build(
					api::model::SpatialModel<CellType>& model,
					api::model::GroupList groups,
					api::model::SpatialAgentFactory<CellType>& factory,
					api::model::SpatialAgentMapping<CellType>& agent_counts
					) override;
	};

	template<typename CellType>
		void SpatialAgentBuilder<CellType>::build(
				api::model::SpatialModel<CellType>& model,
				api::model::GroupList groups,
				api::model::SpatialAgentFactory<CellType>& factory,
				api::model::SpatialAgentMapping<CellType>& agent_counts
				) {
			std::vector<api::model::SpatialAgent<CellType>*> agents;
			for(auto cell : model.cells()) {
				for(std::size_t i = 0; i < agent_counts.countAt(cell); i++) {
					auto agent = factory.build();
					agents.push_back(agent);
					for(api::model::AgentGroup& group : groups)
						group.add(agent);
					agent->initLocation(cell);
				}
			}

			model.runtime().execute(model.distributedMoveAlgorithm().jobs(model, agents, model.cells()));
		}
}}

namespace nlohmann {
	template<typename AgentType, typename CellType, typename Derived>
		struct adl_serializer<fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>>> {
			typedef fpmas::api::utils::PtrWrapper<fpmas::model::SpatialAgentBase<AgentType, CellType, Derived>> Ptr;
			static void to_json(nlohmann::json& j, const Ptr& ptr) {
				// Derived serialization
				j[0] = fpmas::api::utils::PtrWrapper<Derived>(
						const_cast<Derived*>(static_cast<const Derived*>(ptr.get())));
				j[1] = ptr->current_location_id;
			}

			static Ptr from_json(const nlohmann::json& j) {
				// Derived unserialization.
				// The current base is implicitly default initialized
				fpmas::api::utils::PtrWrapper<Derived> derived_ptr
					= j[0].get<fpmas::api::utils::PtrWrapper<Derived>>();

				derived_ptr->current_location_id = j[1].get<fpmas::graph::DistributedId>();
				return derived_ptr.get();
			}
		};

}

#endif
