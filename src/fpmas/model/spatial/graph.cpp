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
	std::size_t base_io<fpmas::model::ReachableCell>::pack_size(
			const fpmas::model::ReachableCell& cell) {
		return datapack::pack_size(cell.reachable_cells);
	}

	void base_io<fpmas::model::ReachableCell>::write(
			DataPack& p, const fpmas::model::ReachableCell& cell, std::size_t& offset) {
		datapack::write(p, cell.reachable_cells, offset);
	}

	void base_io<fpmas::model::ReachableCell>::read(
			const DataPack& p, fpmas::model::ReachableCell& cell, std::size_t& offset) {
		datapack::read(p, cell.reachable_cells, offset);
	}
}}}
