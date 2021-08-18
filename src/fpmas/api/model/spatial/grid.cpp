#include "grid.h"

namespace fpmas {
	std::string to_string(const api::model::DiscretePoint& point) {
		return "(" + std::to_string(point.x) + "," + std::to_string(point.y) + ")";
	}

	namespace api { namespace model {

		float euclidian_distance(const DiscretePoint& p1, const DiscretePoint& p2) {
			return std::sqrt(std::pow(p2.y - p1.y, 2) + std::pow(p2.x - p1.x, 2));
		}

		std::ostream& operator<<(std::ostream& os, const DiscretePoint& point) {
			return os << to_string(point);
		}
	}}
}
