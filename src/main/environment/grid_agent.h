#ifndef GRID_AGENT_H
#define GRID_AGENT_H

#include "grid.h"

namespace FPMAS::environment::grid {

	using agent::TypedPerception;
	template<
		int Range,
		SYNC_MODE,
		int N,
		typename... AgentTypes
	> class GridAgent : public Agent<S, layerCount(N, Range), Cell<Range, S, layerCount(N, Range), AgentTypes...>, AgentTypes...> {
		public:
			typedef Agent<S, layerCount(N, Range), Cell<Range, S, layerCount(N, Range), AgentTypes...>, AgentTypes...> agent_type;
			typedef typename agent_type::node_type node_type;
			typedef typename agent_type::env_type& env_ref;

			void moveTo(
				node_type*,
				env_ref env,
				TypedPerception<node_type, movableTo(N, Range)> target);


	};

	template<int Range, SYNC_MODE, int N, typename... AgentTypes>
		void GridAgent<Range, S, N, AgentTypes...>::moveTo(
				node_type* node,
				env_ref env,
				TypedPerception<node_type, movableTo(N, Range)> target
			) {
			// Unlink current location
			for(auto location : const_cast<const node_type*>(node)
					->layer(locationLayer(N, Range)).getOutgoingArcs()) {
				env.unlink(location);
			}
			// Unlink perceptions
			for(auto perception : const_cast<const node_type*>(node)
					->layer(perceptionsLayer(N, Range)).getOutgoingArcs()){
				env.unlink(perception);
			}

			// Link location
			env.link(node, target.node, locationLayer(N, Range));

			// Unlink movableTo
			for(auto movableToArc : const_cast<const node_type*>(node)
					->layer(movableTo(N, Range)).getOutgoingArcs()) {
				env.unlink(movableToArc);
			}
		}
}

using FPMAS::environment::grid::layerCount;
using FPMAS::environment::grid::GridAgent;
namespace nlohmann {
	template<
		int Range,
		SYNC_MODE,
		int N,
		typename... AgentTypes
	> struct adl_serializer<GridAgent<Range, S, N, AgentTypes...>> {
		static void to_json(json& j, const GridAgent<Range, S, N, AgentTypes...>& data) {

		}

		static void from_json(const json& j, GridAgent<Range, S, N, AgentTypes...>& agent) {

		}

	};
}

#endif
