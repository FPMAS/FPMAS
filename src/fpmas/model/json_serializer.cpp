#include "json_serializer.h"

namespace nlohmann {
	std::unordered_map<FPMAS_TYPE_INDEX, std::type_index> adl_serializer<std::type_index>::id_to_type;
	std::unordered_map<std::type_index, FPMAS_TYPE_INDEX> adl_serializer<std::type_index>::type_to_id;
	FPMAS_TYPE_INDEX adl_serializer<std::type_index>::id = 0;
}
