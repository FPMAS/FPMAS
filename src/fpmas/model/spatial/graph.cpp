#include "graph.h"

namespace nlohmann {

	void adl_serializer<fpmas::model::ReachableCell>::to_json(nlohmann::json &j, const fpmas::model::ReachableCell &cell) {
		j = cell.reachable_cells;
	}

	void adl_serializer<fpmas::model::ReachableCell>::from_json(const nlohmann::json &j, fpmas::model::ReachableCell &cell) {
		cell.reachable_cells = j.get<std::set<fpmas::model::DistributedId>>();
	}
}

namespace fpmas { namespace io { namespace datapack {
	std::size_t Serializer<model::ReachableCell>::size(
			const ObjectPack& p, const model::ReachableCell& cell) {
		return p.size(cell.reachable_cells);
	}

	void Serializer<model::ReachableCell>::to_datapack(
			ObjectPack& p, const model::ReachableCell& cell) {
		p.put(cell.reachable_cells);
	}

	model::ReachableCell Serializer<model::ReachableCell>::from_datapack(
			const ObjectPack& p) {
		model::ReachableCell cell;
		cell.reachable_cells = p.get<std::set<DistributedId>>();
		return cell;
	}
}}}
