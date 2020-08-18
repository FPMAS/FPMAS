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
			void to_json<void>(nlohmann::json& j, const AgentPtr& ptr) {
				FPMAS_LOGE(-1, "AGENT_SERIALIZER",
						"Invalid agent type : %lu. Make sure to properly register \
						the Agent type with FPMAS_JSON_SET_UP and FPMAS_REGISTER_AGENT_TYPES.",
						ptr->typeId());
				std::string message = "Invalid agent type : " + std::string(ptr->typeId().name());
				throw std::invalid_argument(message);
			}

		template<> 
			AgentPtr from_json<void>(const nlohmann::json& j) {
				fpmas::api::model::TypeId id = j.at("type").get<fpmas::api::model::TypeId>();
				FPMAS_LOGE(-1, "AGENT_SERIALIZER", "Invalid agent type : %lu", id);
				std::string message = "Invalid agent type : " + std::string(id.name());
				throw std::invalid_argument(message);
			}
	}
}
