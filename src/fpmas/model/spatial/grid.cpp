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

namespace fpmas { namespace io { namespace datapack {
	std::size_t Serializer<api::model::DiscretePoint>::size(const ObjectPack& p) {
		return 2*p.size<api::model::DiscreteCoordinate>();
	}
	std::size_t Serializer<api::model::DiscretePoint>::size(
			const ObjectPack& p, const api::model::DiscretePoint&) {
		return size(p);
	}

	void Serializer<api::model::DiscretePoint>::to_datapack(
			ObjectPack& pack, const api::model::DiscretePoint& point) {
		pack.put(point.x);
		pack.put(point.y);
	}

	api::model::DiscretePoint Serializer<api::model::DiscretePoint>::from_datapack(
			const ObjectPack& pack) {
		return {
			pack.get<api::model::DiscreteCoordinate>(),
			pack.get<api::model::DiscreteCoordinate>()
		};
	}
}}}

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
