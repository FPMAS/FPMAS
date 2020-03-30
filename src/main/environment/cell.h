#ifndef CELL_H
#define CELL_H

#include "agent/agent.h"

using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {
		static constexpr LayerId locationLayer(int, int);
		static constexpr LayerId movableTo(int, int);
		static constexpr LayerId perceptionsLayer(int, int);
		static constexpr LayerId perceivableFromLayer(int, int);

		template<int Range, SYNC_MODE, int N, typename... AgentTypes> class Cell
			: public Agent<S, N, Cell<Range, S, N, AgentTypes...>, AgentTypes...> {
				public:
					typedef Environment<S, N, Cell<Range, S, N, AgentTypes...>, AgentTypes...> env_type;
					typedef typename env_type::node_ptr node_ptr;
					typedef typename env_type::node_type node_type;

				private:
					const int _x = 0;
					const int _y = 0;
					std::set<DistributedId> upToDateAgents;

				public:
					Cell(int x, int y)
						: _x(x), _y(y) {}

					Cell(const Cell<Range, S, N, AgentTypes...>& other) : Cell(other._x, other._y) {}

					void act(node_ptr, env_type&) override;

					const int x() const {
						return _x;
					}
					const int y() const {
						return _y;
					}
			};

		template<int Range, SYNC_MODE, int N, typename... AgentTypes>
			void Cell<Range, S, N, AgentTypes...>::act(node_ptr cellNode, env_type& env) {
			std::set<DistributedId> currentAgents;
			for(auto locationPerception : this->template perceptions<locationLayer(N, Range)>(cellNode).get()) {
				DistributedId nodeId = locationPerception.node->getId();
				currentAgents.insert(nodeId);
				if(upToDateAgents.count(nodeId) == 0) {
					for(auto perceivableFromPerception : this->template perceptions<perceivableFromLayer(N, Range)>(cellNode).get()) {
						env.link(locationPerception.node, perceivableFromPerception.node, perceptionsLayer(N, Range));
					}
					for(auto otherAgentPerception : this->template perceptions<movableTo(N, Range)>(cellNode).get()) {
						// For now, assumes perceive == movableTo
						env.link(otherAgentPerception.node, locationPerception.node, perceptionsLayer(N, Range));
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
	template <int Range, SYNC_MODE, int N, typename... AgentTypes>
		struct adl_serializer<Cell<Range, S, N, AgentTypes...>> {
			static void to_json(json& j, const Cell<Range, S, N, AgentTypes...>& value) {
				j["x"] = value.x();
				j["y"] = value.y();
			}

			static Cell<Range, S, N, AgentTypes...> from_json(const json& j) {
				return Cell<Range, S, N, AgentTypes...>(j.at("x").get<int>(), j.at("y").get<int>());
			}
		};
}
#endif
