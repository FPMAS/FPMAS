#ifndef CELL_H
#define CELL_H

#include "agent/agent.h"

using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {
		static constexpr LayerId locationLayer(int);
		static constexpr LayerId movableTo(int);
		static constexpr LayerId perceptionsLayer(int);
		static constexpr LayerId perceivableFromLayer(int);

		template<int Range, SYNC_MODE, typename... AgentTypes> class Cell
			: public Agent<S, Cell<Range, S, AgentTypes...>, AgentTypes...> {
				public:
					typedef Environment<S, Cell<Range, S, AgentTypes...>, AgentTypes...> env_type;
					typedef typename env_type::node_ptr node_ptr;
					typedef typename env_type::node_type node_type;

				private:
					const int _x = 0;
					const int _y = 0;
					std::set<DistributedId> upToDateAgents;

				public:
					Cell(int x, int y)
						: _x(x), _y(y) {}

					Cell(const Cell<Range, S, AgentTypes...>& other) : Cell(other._x, other._y) {}

					void act(node_ptr, env_type&) override;

					const int x() const {
						return _x;
					}
					const int y() const {
						return _y;
					}
			};

		template<int Range, SYNC_MODE, typename... AgentTypes>
			void Cell<Range, S, AgentTypes...>::act(node_ptr cellNode, env_type& env) {
			std::set<DistributedId> currentAgents;
			for(auto locationPerception : this->template perceptions<locationLayer(Range)>(cellNode).get()) {
				DistributedId nodeId = locationPerception.node->getId();
				currentAgents.insert(nodeId);
				if(upToDateAgents.count(nodeId) == 0) {
					for(auto perceivableFromPerception : this->template perceptions<perceivableFromLayer(Range)>(cellNode).get()) {
						env.link(locationPerception.node, perceivableFromPerception.node, perceptionsLayer(Range));
					}
					for(auto otherAgentPerception : this->template perceptions<movableTo(Range)>(cellNode).get()) {
						// For now, assumes perceive == movableTo
						env.link(otherAgentPerception.node, locationPerception.node, perceptionsLayer(Range));
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
	template <int Range, SYNC_MODE, typename... AgentTypes>
		struct adl_serializer<Cell<Range, S, AgentTypes...>> {
			static void to_json(json& j, const Cell<Range, S, AgentTypes...>& value) {
				j["x"] = value.x();
				j["y"] = value.y();
			}

			static Cell<Range, S, AgentTypes...> from_json(const json& j) {
				return Cell<Range, S, AgentTypes...>(j.at("x").get<int>(), j.at("y").get<int>());
			}
		};
}
#endif
