#ifndef GRID_AGENT_H
#define GRID_AGENT_H

#include "agent/agent.h"
#include "agent/perceptions.h"

namespace FPMAS::environment::grid {
	template<
		SYNC_MODE,
		typename... AgentTypes
		> class Grid;

	template<SYNC_MODE, typename... AgentTypes> class Cell;
	using agent::Perception;
	template<
		SYNC_MODE,
		typename... AgentTypes
	> class GridAgent : public Agent<S, Cell<S, AgentTypes...>, AgentTypes...> {
		public:
			typedef Agent<S, Cell<S, AgentTypes...>, AgentTypes...> agent_type;
			typedef Grid<S, AgentTypes...> grid_type;
			typedef typename agent_type::node_type node_type;
			typedef typename agent_type::env_type& env_ref;

			const int perceptionRange;
			const int movableRange;

			GridAgent(int movableRange, int perceptionRange)
				: movableRange(movableRange), perceptionRange(perceptionRange) {}

			void moveTo(
				node_type*,
				env_ref env,
				Perception<node_type, MOVABLE_TO> target);

	};

	template<SYNC_MODE, typename... AgentTypes>
		void GridAgent<S, AgentTypes...>::moveTo(
				node_type* node,
				env_ref env,
				Perception<node_type, MOVABLE_TO> target
			) {
			// Unlink current location
			for(auto location : node
					->layer(LOCATION).getOutgoingArcs()) {
				env.unlink(location);
			}
			// Unlink perceptions
			for(auto perception : node
					->layer(PERCEPTIONS).getOutgoingArcs()){
				env.unlink(perception);
			}

			for(auto perceive : node
					->layer(PERCEIVE).getOutgoingArcs()){
				env.unlink(perceive);
			}

			auto& grid
				= static_cast<const grid_type&>(env);
			for (int i = 1; i <= grid.neighborhoodRange; i++) {
				for(auto perceivableFrom : node
						->layer(PERCEIVABLE_FROM(i)).getOutgoingArcs()){
					env.unlink(perceivableFrom);
				}
			}

			// Link location
			// Should be performed before unlinking MOVABLE_TO, otherwise the
			// target might be deleted
			env.link(node, target.node(), LOCATION);

			// Unlink movableTo
			for(auto movableToArc : node
					->layer(MOVABLE_TO).getOutgoingArcs()) {
				env.unlink(movableToArc);
			}
		}
}

using FPMAS::environment::grid::GridAgent;
namespace nlohmann {
	template<
		SYNC_MODE,
		typename... AgentTypes
	> struct adl_serializer<GridAgent<S, AgentTypes...>> {
		static void to_json(json& j, const GridAgent<S, AgentTypes...>& data) {

		}

		static void from_json(const json& j, GridAgent<S, AgentTypes...>& agent) {

		}

	};
}

#endif
