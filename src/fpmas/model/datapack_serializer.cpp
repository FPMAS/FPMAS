#include "datapack_serializer.h"

namespace fpmas { namespace io { namespace datapack {
	std::unordered_map<std::size_t, std::type_index> Serializer<std::type_index>::id_to_type;
	std::unordered_map<std::type_index, std::size_t> Serializer<std::type_index>::type_to_id;
	std::size_t Serializer<std::type_index>::id = 0;
}}}
