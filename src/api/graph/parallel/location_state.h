#ifndef LOCATION_STATE_H
#define LOCATION_STATE_H

#include <iostream>

namespace fpmas::api::graph::parallel {
	enum LocationState {
		LOCAL,
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
}
#endif
