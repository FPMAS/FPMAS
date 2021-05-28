#include "grid.h"
#include "fpmas/random/generator.h"

namespace fpmas { namespace model {
	std::size_t PointHash::operator()(const DiscretePoint& p) const {
		std::size_t hash = std::hash<DiscreteCoordinate>()(p.x);
		hash ^= std::hash<DiscreteCoordinate>()(p.y) + 0x9e3779b9
			+ (hash << 6) + (hash >> 2);
		return hash;
	}
}}

namespace std {
	std::size_t hash<fpmas::model::DiscretePoint>::operator()(const fpmas::model::DiscretePoint& p) const {
		return hasher(p);
	}
	const fpmas::model::PointHash hash<fpmas::model::DiscretePoint>::hasher;
}
namespace fpmas { namespace api { namespace model {
	bool operator==(const DiscretePoint& p1, const DiscretePoint& p2) {
		return p1.x == p2.x && p1.y == p2.y;
	}
}}}
