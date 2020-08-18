#ifndef FPMAS_LOCATION_STATE_H
#define FPMAS_LOCATION_STATE_H

/** \file src/fpmas/api/graph/location_state.h
 * LocationState implementation.
 */

#include <iostream>

namespace fpmas { namespace api { namespace graph {
	/**
	 * Enum describing the current state of a DistributedNode or a
	 * DistributedEdge.
	 */
	enum LocationState {
		/**
		 * A DistributedNode is LOCAL iff it is currently hosted and managed by the current
		 * process.
		 * A DistributedEdge is LOCAL iff its source and target nodes are LOCAL.
		 */
		LOCAL,
		/**
		 * A DistributedNode is DISTANT iff it is a representation of a Node currently
		 * hosted by an other process.
		 * A DistributedEdge is DISTANT iff at least of its target and source
		 * nodes is DISTANT.
		 */
		DISTANT
	};

	inline std::ostream& operator<<(std::ostream& os, const LocationState& loc) {
		switch(loc) {
			case LOCAL:
				os << "LOCAL";
				break;
			case DISTANT:
				os << "DISTANT";
				break;
			default:
				os << "UNDEFINED";
		}
		return os;
	}
}}}
#endif
