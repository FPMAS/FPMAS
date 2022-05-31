#include "grid.h"
#include "fpmas/random/generator.h"

namespace fpmas { namespace model {
	std::size_t PointHash::operator()(const DiscretePoint& p) const {
		std::size_t hash = std::hash<DiscreteCoordinate>()(p.x);
		hash ^= std::hash<DiscreteCoordinate>()(p.y) + 0x9e3779b9
			+ (hash << 6) + (hash >> 2);
		return hash;
	}

	random::mt19937_64 RandomGridAgentBuilder::rd;

	void RandomGridAgentBuilder::seed(random::mt19937_64::result_type seed) {
		RandomGridAgentBuilder::rd.seed(seed);
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
		api::model::DiscreteCoordinate x = pack.get<api::model::DiscreteCoordinate>();
		api::model::DiscreteCoordinate y = pack.get<api::model::DiscreteCoordinate>();
		return {x, y};
	}
}}}

namespace std {
	std::size_t hash<fpmas::model::DiscretePoint>::operator()(const fpmas::model::DiscretePoint& p) const {
		return hasher(p);
	}
	const fpmas::model::PointHash hash<fpmas::model::DiscretePoint>::hasher;
}
