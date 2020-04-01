#ifndef CELL_H
#define CELL_H

#include "grid_agent.h"
#include "grid_layers.h"

using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {
	class CellBase {
		public:
			const int x;
			const int y;

			CellBase(int x, int y) : x(x), y(y) {}
	};

	template<SYNC_MODE, typename...> class Grid;

	template<SYNC_MODE, typename... AgentTypes> class Cell
		: public Agent<S, Cell<S, AgentTypes...>, AgentTypes...>, public CellBase {
			public:
				typedef Environment<S, Cell<S, AgentTypes...>, AgentTypes...> env_type;
				typedef typename env_type::node_ptr node_ptr;
				typedef typename env_type::node_type node_type;
				typedef GridAgent<S, AgentTypes...> grid_agent_type;
				typedef Grid<S, AgentTypes...> grid_type;

			private:
				std::set<DistributedId> upToDateAgents;

			public:
				Cell(int x, int y)
					: CellBase(x, y) {}

				Cell(const Cell<S, AgentTypes...>& other) : Cell(other._x, other._y) {}

				void act(node_ptr, env_type&) override;

		};

	template<SYNC_MODE, typename... AgentTypes>
		void Cell<S, AgentTypes...>::act(node_ptr cellNode, env_type& env) {
			std::set<DistributedId> currentAgents;
			// For agent located on this cell
			for(auto localAgentLink : 
				cellNode->layer(LOCATION).getIncomingArcs()) {

				DistributedId nodeId = localAgentLink->getSourceNode()->getId();
				currentAgents.insert(nodeId);

				// If the agent has not been updated yet
				if(upToDateAgents.count(nodeId) == 0) {
					// Reads the agent to get its perception and movable range
					auto agent = static_cast<const grid_agent_type*>(
							localAgentLink->getSourceNode()->data()->read().get()
							);
					// Updates agent perceptions with items perveivable from
					// this cell
					for(auto otherLocalAgent :
							cellNode->layer(LOCATION).getIncomingArcs()) {
						if(otherLocalAgent != localAgentLink) {
							env.link(
								localAgentLink->getSourceNode(),
								otherLocalAgent->getSourceNode(),
								PERCEPTIONS);
							env.link(
								otherLocalAgent->getSourceNode(),
								localAgentLink->getSourceNode(),
								PERCEPTIONS);
						}
					}
					for (int i = 1; i <= agent->perceptionRange; i++) {
						for(auto perceivableFromLink :
								cellNode->layer(PERCEIVABLE_FROM(i)).getIncomingArcs()) {
							env.link(
									localAgentLink->getSourceNode(),
									perceivableFromLink->getSourceNode(),
									PERCEPTIONS);
						}	
					}
					// Adds the current agent to perceptions of agents that are
					// perceiving this cell
					for(auto perceiveAgentLink :
						cellNode->layer(PERCEIVE).getIncomingArcs()) {
						env.link(
							perceiveAgentLink->getSourceNode(),
							localAgentLink->getSourceNode(),
							PERCEPTIONS);
					}
					// Updates the current agent movableTo links with cell
					// neighbors
					for (int i = 1; i <= agent->movableRange; i++) {
						for(auto cellNeighbor :
						cellNode->layer(NEIGHBOR_CELL(i)).outNeighbors()) {
						env.link(
							localAgentLink->getSourceNode(),
							cellNeighbor,
							MOVABLE_TO);
						}
					}
					for (int i = 1; i <= agent->perceptionRange; i++) {
						for(auto cellNeighbor :
								cellNode->layer(NEIGHBOR_CELL(i)).outNeighbors()) {
							env.link(
									localAgentLink->getSourceNode(),
									cellNeighbor,
									PERCEIVE);
						}
					}
					// Makes the agent perveivableFrom the neighbor cells
					auto& grid
						= static_cast<const grid_type&>(env);
					for (int i = 1; i <= grid.neighborhoodRange; i++) {
						for(auto cellNeighbor :
								cellNode->layer(NEIGHBOR_CELL(i)).outNeighbors()) {
							env.link(
									localAgentLink->getSourceNode(),
									cellNeighbor,
									PERCEIVABLE_FROM(i));
						}
					}
					upToDateAgents.insert(nodeId);
				}
			}
			for(auto it = upToDateAgents.begin(); it!=upToDateAgents.end();) {
				if(currentAgents.count(*it) == 0) {
					it = upToDateAgents.erase(it);
				} else {
					it++;
				}
			}
		}
}

namespace nlohmann {
	using FPMAS::environment::grid::Cell;
	template <SYNC_MODE, typename... AgentTypes>
		struct adl_serializer<Cell<S, AgentTypes...>> {
			static void to_json(json& j, const Cell<S, AgentTypes...>& value) {
				j["x"] = value.x;
				j["y"] = value.y;
			}

			static Cell<S, AgentTypes...> from_json(const json& j) {
				return Cell<S, AgentTypes...>(j.at("x").get<int>(), j.at("y").get<int>());
			}
		};
}
#endif
