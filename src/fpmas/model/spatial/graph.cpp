#include "graph.h"

namespace nlohmann {

	void adl_serializer<fpmas::model::ReachableCell>::to_json(nlohmann::json &j, const fpmas::model::ReachableCell &cell) {
		j = cell.reachable_cells;
	}

	void adl_serializer<fpmas::model::ReachableCell>::from_json(const nlohmann::json &j, fpmas::model::ReachableCell &cell) {
		cell.reachable_cells = j.get<std::set<fpmas::model::DistributedId>>();
	}
}
