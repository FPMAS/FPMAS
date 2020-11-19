#include "serializer.h"

namespace nlohmann {
	std::unordered_map<std::size_t, std::type_index> adl_serializer<std::type_index>::id_to_type;
	std::unordered_map<std::type_index, std::size_t> adl_serializer<std::type_index>::type_to_id;
	std::size_t adl_serializer<std::type_index>::id = 0;
}

namespace fpmas {

	template<>
		void register_types<void>() {}

	namespace model {
		template<> 
			void to_json<void>(nlohmann::json&, const AgentPtr& ptr) {
				FPMAS_LOGE(-1, "AGENT_SERIALIZER",
						"Invalid agent type : %s. Make sure to properly register \
						the Agent type with FPMAS_JSON_SET_UP and FPMAS_REGISTER_AGENT_TYPES.",
						ptr->typeId().name());
				throw exceptions::BadTypeException(ptr->typeId());
			}

		template<> 
			AgentPtr from_json<void>(const nlohmann::json& j) {
				std::size_t id = j.at("type").get<std::size_t>();
				FPMAS_LOGE(-1, "AGENT_SERIALIZER",
						"Invalid agent type id : %lu. Make sure to properly register \
						the Agent type with FPMAS_JSON_SET_UP and FPMAS_REGISTER_AGENT_TYPES.",
						id);
				throw exceptions::BadIdException(id);
			}
	}
}
