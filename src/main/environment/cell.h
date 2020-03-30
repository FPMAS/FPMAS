#ifndef CELL_H
#define CELL_H

#include "agent/agent.h"
#include "grid_layers.h"

using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {
	class CellBase {
		public:
			const int x;
			const int y;

			CellBase(int x, int y) : x(x), y(y) {}
	};

	template<SYNC_MODE, typename... AgentTypes> class Cell
		: public Agent<S, Cell<S, AgentTypes...>, AgentTypes...>, public CellBase {
			public:
				typedef Environment<S, Cell<S, AgentTypes...>, AgentTypes...> env_type;
				typedef typename env_type::node_ptr node_ptr;
				typedef typename env_type::node_type node_type;

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
			for(auto locationPerception : this->template perceptions<LOCATION>(cellNode).get()) {
				DistributedId nodeId = locationPerception.node->getId();
				currentAgents.insert(nodeId);
				if(upToDateAgents.count(nodeId) == 0) {
					for(auto perceivableFromPerception : this->template perceptions<PERCEIVABLE_FROM>(cellNode).get()) {
						env.link(locationPerception.node, perceivableFromPerception.node, PERCEPTIONS);
					}
					for(auto otherAgentPerception : this->template perceptions<MOVABLE_TO>(cellNode).get()) {
						// For now, assumes perceive == movableTo
						env.link(otherAgentPerception.node, locationPerception.node, PERCEPTIONS);
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
