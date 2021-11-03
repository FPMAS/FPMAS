#include "json_serializer.h"

namespace nlohmann {
	std::unordered_map<std::size_t, std::type_index> adl_serializer<std::type_index>::id_to_type;
	std::unordered_map<std::type_index, std::size_t> adl_serializer<std::type_index>::type_to_id;
	std::size_t adl_serializer<std::type_index>::id = 0;
}
