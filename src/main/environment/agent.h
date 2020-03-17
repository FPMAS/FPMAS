#ifndef GRID_AGENT_H
#define GRID_AGENT_H

#include "graph/base/graph.h"
#include "../agent/agent.h"

using FPMAS::graph::base::Node;
using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {

	template<int N, typename... Types> class GridAgent : public Agent<N, Types...> {
		public:
			Node<std::unique_ptr<Agent<N, Types...>>, N>* location();
			void moveTo(Node<std::unique_ptr<Agent<N, Types...>>, N>*);
			void moveTo(int x, int y);


	};

	template<int N, typename... Types> Node<std::unique_ptr<Agent<N, Types...>>, N>*
		GridAgent<N, Types...>::location() {
			//return this->node()->layer(Grid::locationLayer).outNeighbors().at(0);
		}
}

#endif
