#ifndef CELL_H
#define CELL_H

#include "agent/agent.h"

using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {
		template<SYNC_MODE, int N, typename... AgentTypes> class Cell
			: public Agent<S, N, Cell<S, N, AgentTypes...>, AgentTypes...> {
				public:
					typedef Environment<S, N, Cell<S, N, AgentTypes...>, AgentTypes...> env_type;
					typedef typename env_type::node_ptr node_ptr;

				private:
					const int _x = 0;
					const int _y = 0;
					void act(node_ptr, env_type&) override {};

				public:
					Cell(int x, int y)
						: _x(x), _y(y) {}

					Cell(const Cell<S, N, AgentTypes...>& other) : Cell(other._x, other._y) {}

					const int x() const {
						return _x;
					}
					const int y() const {
						return _y;
					}
			};

}
#endif
