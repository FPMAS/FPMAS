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
	> class GridAgent : public Agent<S, layerCount(N, Range), Cell<S, layerCount(N, Range), AgentTypes...>, AgentTypes...> {
		public:
			typedef Agent<S, layerCount(N, Range), Cell<S, layerCount(N, Range), AgentTypes...>, AgentTypes...> agent_type;
			typedef typename agent_type::node_type node_type;
			typedef typename agent_type::env_type& env_ref;

			void moveTo(
				const node_type* node,
				env_ref env,
				TypedPerception<node_type, movableTo(N, Range)> target);


	};

	template<int Range, SYNC_MODE, int N, typename... AgentTypes>
		void GridAgent<Range, S, N, AgentTypes...>::moveTo(
				const node_type* node,
				env_ref env,
				TypedPerception<node_type, movableTo(N, Range)> target
			) {
			// Unlink current location
			for(auto location : node->layer(locationLayer(N, Range)).getOutgoingArcs()) {
				env.unlink(location);
			}
			// Unlink perceptions
			for(auto perception : node->layer(perceptions(N, Range)).getOutgoingArcs()){
				env.unlink(perception);
			}

			// Link location
			// TODO : I need id management...
			//env.link(node->getId(), target.node->getIg(), ???, locationLayer(N, Range));
		}
}

#endif
