#ifndef GRID_AGENT_H
#define GRID_AGENT_H

#include "grid.h"

namespace FPMAS::environment::grid {

	using agent::TypedPerception;
	template<
		int Range,
		SYNC_MODE,
		typename... AgentTypes
	> class GridAgent : public Agent<S, Cell<Range, S, AgentTypes...>, AgentTypes...> {
		public:
			typedef Agent<S, Cell<Range, S, AgentTypes...>, AgentTypes...> agent_type;
			typedef typename agent_type::node_type node_type;
			typedef typename agent_type::env_type& env_ref;

			void moveTo(
				node_type*,
				env_ref env,
				TypedPerception<node_type, movableTo(Range)> target);


	};

	template<int Range, SYNC_MODE, typename... AgentTypes>
		void GridAgent<Range, S, AgentTypes...>::moveTo(
				node_type* node,
				env_ref env,
				TypedPerception<node_type, movableTo(Range)> target
			) {
			// Unlink current location
			for(auto location : const_cast<const node_type*>(node)
					->layer(locationLayer(Range)).getOutgoingArcs()) {
				env.unlink(location);
			}
			// Unlink perceptions
			for(auto perception : const_cast<const node_type*>(node)
					->layer(perceptionsLayer(Range)).getOutgoingArcs()){
				env.unlink(perception);
			}

			// Link location
			env.link(node, target.node, locationLayer(Range));

			// Unlink movableTo
			for(auto movableToArc : const_cast<const node_type*>(node)
					->layer(movableTo(Range)).getOutgoingArcs()) {
				env.unlink(movableToArc);
			}
		}
}

using FPMAS::environment::grid::GridAgent;
namespace nlohmann {
	template<
		int Range,
		SYNC_MODE,
		typename... AgentTypes
	> struct adl_serializer<GridAgent<Range, S, AgentTypes...>> {
		static void to_json(json& j, const GridAgent<Range, S, AgentTypes...>& data) {

		}

		static void from_json(const json& j, GridAgent<Range, S, AgentTypes...>& agent) {

		}

	};
}

#endif
