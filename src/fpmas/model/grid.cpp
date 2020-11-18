#include "grid.h"

namespace fpmas { namespace model {

	void VonNeumannNeighborhood::buildLocalGrid(
			api::model::Model& model,
			api::model::Environment& environment,
			DiscreteCoordinate min_x, DiscreteCoordinate max_x,
			DiscreteCoordinate min_y, DiscreteCoordinate max_y) {

		DiscreteCoordinate local_height = max_y - min_y + 1;
		DiscreteCoordinate local_width = max_x - min_x + 1;
		fpmas::api::model::GridCell*** cells = (fpmas::api::model::GridCell***)
			std::malloc(local_height * sizeof(GridCell**));
		for(DiscreteCoordinate y = 0; y < local_height; y++)
			cells[y] = (fpmas::api::model::GridCell**)
				std::malloc(local_width * sizeof(fpmas::api::model::GridCell*));

		for(DiscreteCoordinate y = min_y; y <= max_y; y++) {
			for(DiscreteCoordinate x = min_x; x <= max_x; x++) {
				auto cell = cell_factory.build({x, y});
				cells[y-min_y][x-min_x] = cell;
				environment.add(cell);
			}
		}
		for(DiscreteCoordinate j = 0; j < local_width; j++) {
			for(DiscreteCoordinate i = 0; i < local_height-1; i++) {
				model.link(cells[i][j], cells[i+1][j], api::model::NEIGHBOR_CELL);
			}
			for(DiscreteCoordinate i = local_height-1; i > 0; i--) {
				model.link(cells[i][j], cells[i-1][j], api::model::NEIGHBOR_CELL);
			}
		}
		for(DiscreteCoordinate i = 0; i < local_height; i++) {
			for(DiscreteCoordinate j = 0; j < local_width-1; j++) {
				model.link(cells[i][j], cells[i][j+1], api::model::NEIGHBOR_CELL);
			}
			for(DiscreteCoordinate j = local_width-1; j > 0; j--) {
				model.link(cells[i][j], cells[i][j-1], api::model::NEIGHBOR_CELL);
			}
		}

		for(DiscreteCoordinate i = 0; i < local_height; i++)
			std::free(cells[i]);
		std::free(cells);

	}

	void VonNeumannNeighborhood::build(
			api::model::Model& model,
			api::model::Environment& environment) {
		
		FPMAS_ON_PROC(model.graph().getMpiCommunicator(), 0) {
			buildLocalGrid(model, environment, 0, width-1, 0, height-1);
		}
	}
}}

namespace fpmas { namespace api { namespace model {
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2) {
		return p1.x == p2.x && p1.y == p2.y;
	}
}}}
