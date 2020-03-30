#ifndef GRID_AGENT_H
#define GRID_AGENT_H

#include "grid.h"

namespace FPMAS::environment::grid {

	using agent::TypedPerception;
	template<
		SYNC_MODE,
		typename... AgentTypes
	> class GridAgent : public Agent<S, Cell<S, AgentTypes...>, AgentTypes...> {
		public:
			typedef Agent<S, Cell<S, AgentTypes...>, AgentTypes...> agent_type;
			typedef typename agent_type::node_type node_type;
			typedef typename agent_type::env_type& env_ref;

			void moveTo(
				node_type*,
				env_ref env,
				TypedPerception<node_type, MOVABLE_TO> target);


	};

	template<SYNC_MODE, typename... AgentTypes>
		void GridAgent<S, AgentTypes...>::moveTo(
				node_type* node,
				env_ref env,
				TypedPerception<node_type, MOVABLE_TO> target
			) {
			// Unlink current location
			for(auto location : const_cast<const node_type*>(node)
					->layer(LOCATION).getOutgoingArcs()) {
				env.unlink(location);
			}
			// Unlink perceptions
			for(auto perception : const_cast<const node_type*>(node)
					->layer(PERCEPTIONS).getOutgoingArcs()){
				env.unlink(perception);
			}

			// Link location
			env.link(node, target.node, LOCATION);

			// Unlink movableTo
			for(auto movableToArc : const_cast<const node_type*>(node)
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
