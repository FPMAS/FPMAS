#include "grid.h"

namespace fpmas {
	std::string to_string(const api::model::DiscretePoint& point) {
		return "(" + std::to_string(point.x) + "," + std::to_string(point.y) + ")";
	}

	namespace api { namespace model {
		std::ostream& operator<<(std::ostream& os, const DiscretePoint& point) {
			return os << to_string(point);
		}
	}}
}
