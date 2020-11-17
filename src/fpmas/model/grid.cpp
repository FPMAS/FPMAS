#include "grid.h"

namespace fpmas { namespace model {

	void VonNeumannNeighborhood::build(
			api::model::Model& model,
			api::model::Environment& environment) {
		
		FPMAS_ON_PROC(model.graph().getMpiCommunicator(), 0) {
			fpmas::api::model::GridCell*** cells = (fpmas::api::model::GridCell***)
				std::malloc(this->height * sizeof(GridCell**));
			for(DiscreteCoordinate i = 0; i < this->height; i++)
				cells[i] = (fpmas::api::model::GridCell**)
					std::malloc(this->width * sizeof(fpmas::api::model::GridCell*));

			for(DiscreteCoordinate i = 0; i < this->height; i++) {
				for(DiscreteCoordinate j = 0; j < this->width; j++) {
					cells[i][j] = cell_factory.build({i, j});
					environment.add(cells[i][j]);
				}
			}
			for(DiscreteCoordinate j = 0; j < this->width; j++) {
				for(DiscreteCoordinate i = 0; i < this->height-1; i++) {
					model.link(cells[i][j], cells[i+1][j], api::model::NEIGHBOR_CELL);
				}
				for(DiscreteCoordinate i = this->height-1; i > 0; i--) {
					model.link(cells[i][j], cells[i-1][j], api::model::NEIGHBOR_CELL);
				}
			}
			for(DiscreteCoordinate i = 0; i < this->height; i++) {
				for(DiscreteCoordinate j = 0; j < this->width-1; j++) {
					model.link(cells[i][j], cells[i][j+1], api::model::NEIGHBOR_CELL);
				}
				for(DiscreteCoordinate j = this->width-1; j > 0; j--) {
					model.link(cells[i][j], cells[i][j-1], api::model::NEIGHBOR_CELL);
				}
			}

			for(DiscreteCoordinate i = 0; i < this->height; i++)
				std::free(cells[i]);
			std::free(cells);
		}
	}
}}

namespace fpmas { namespace api { namespace model {
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2) {
		return p1.x == p2.x && p1.y == p2.y;
	}
}}}
