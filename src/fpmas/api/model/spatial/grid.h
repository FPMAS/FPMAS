#ifndef FPMAS_GRID_API_H
#define FPMAS_GRID_API_H

/** \file src/fpmas/api/model/spatial/grid.h
 * Grid models API.
 */

#include "spatial_model.h"

namespace fpmas { namespace api { namespace model {

	/**
	 * Integer coordinate type.
	 */
	typedef long DiscreteCoordinate;

	/**
	 * A structure representing a 2D discrete point (i.e. with integer
	 * coordinates).
	 */
	struct DiscretePoint {
		/**
		 * X coordinate.
		 */
		DiscreteCoordinate x;
		/**
		 * Y coordinate.
		 */
		DiscreteCoordinate y;

		/**
		 * Default constructor.
		 *
		 * Coordinates are default initialized.
		 */
		DiscretePoint() {}
		/**
		 * DiscretePoint constructor.
		 *
		 * @param x X coordinate
		 * @param y Y coordinate
		 */
		DiscretePoint(DiscreteCoordinate x, DiscreteCoordinate y)
			: x(x), y(y) {}
	};

	/**
	 * Cell API extension to represent a Cell in a Grid.
	 *
	 * Contrary to base \Cells that do not need to be explicitly situated,
	 * \GridCells are situated using a DiscretePoint.
	 */
	class GridCell : public virtual Cell {
		public:
			/**
			 * Returns the GridCell location.
			 *
			 * @return grid cell location
			 */
			virtual DiscretePoint location() const = 0;
	};

	/**
	 * SpatialAgent API extension to represent agents moving on a Grid.
	 *
	 * The GridAgent notably provides methods to move or locate the agent using
	 * DiscretePoint coordinates.
	 */
	class GridAgent : public virtual SpatialAgent<GridCell> {
		protected:
			using SpatialAgent::moveTo;

			/**
			 * Moves to the Cell at the specified point.
			 *
			 * Notice that it is assumed that at most one GridCell can be
			 * located at each `point`.
			 *
			 * If no Cell at `point` can be found in the current
			 * mobilityField(), an OutOfMobilityFieldException is thrown.
			 *
			 * @param point discrete coordinates
			 * @throw OutOfMobilityFieldException
			 */
			virtual void moveTo(DiscretePoint point) = 0;

		public:
			/**
			 * Returns the current location of the GridAgent as discrete
			 * coordinates.
			 *
			 * The behavior of this method is similar to the
			 * SpatialAgent::locationId() method: considering the
			 * moveTo(DiscretePoint) method, the agent can move using this kind
			 * of operations:
			 * ```cpp
			 * agent->moveTo(neighbor->locationPoint())
			 * ```
			 *
			 * @return location of the GridAgent
			 */
			virtual DiscretePoint locationPoint() const = 0;
	};

	/**
	 * SpatialModel specialization for grid based Models.
	 */
	typedef SpatialModel<GridCell> GridModel;

	/**
	 * GridCellFactory API.
	 */
	class GridCellFactory {
		public:
			/**
			 * Returns a pointer to a dynamically allocated GridCell, located
			 * at the specified `location`.
			 *
			 * More formally, the built GridCell is such that
			 * `grid_cell->location() == location`.
			 *
			 * @param location grid cell location
			 * @return pointer to dynamically allocated GridCell
			 */
			virtual GridCell* build(DiscretePoint location) = 0;

			virtual ~GridCellFactory() {}
	};
}}}

namespace fpmas {
	/**
	 * DiscretePoint string conversion.
	 *
	 * @param point point to convert
	 * @return string representation of the point
	 */
	std::string to_string(const api::model::DiscretePoint& point);

	namespace api { namespace model {
		/**
		 * DiscretePoint stream output operator.
		 *
		 * @param os output stream
		 * @param point point to insert
		 * @return os
		 */
		std::ostream& operator<<(std::ostream& os, const DiscretePoint& point);
	}}
}
#endif
