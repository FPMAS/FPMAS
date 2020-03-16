#ifndef CELL_H
#define CELL_H

#include "agent/agent.h"

using FPMAS::agent::Agent;

namespace FPMAS::environment::grid {
		class Cell : public Agent<Cell> {
			private:
				const int _x;
				const int _y;
				void act() override {};
			public:
				Cell() : _x(0), _y(0) {}
				Cell(int x, int y) : _x(x), _y(y) {}

				Cell(const Cell& other) : Cell(other._x, other._y) {}

				const int x() const {
					return _x;
				}
				const int y() const {
					return _y;
				}
		};

}
#endif
