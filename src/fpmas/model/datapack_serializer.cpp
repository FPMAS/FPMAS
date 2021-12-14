#include "datapack_serializer.h"

namespace fpmas { namespace io { namespace datapack {
	std::unordered_map<FPMAS_TYPE_INDEX, std::type_index> Serializer<std::type_index>::id_to_type;
	std::unordered_map<std::type_index, FPMAS_TYPE_INDEX> Serializer<std::type_index>::type_to_id;
	FPMAS_TYPE_INDEX Serializer<std::type_index>::id = 0;
}}}
