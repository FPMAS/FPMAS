#ifndef AGENT_SERIALIZER_H
#define AGENT_SERIALIZER_H

#include <nlohmann/json.hpp>
#include <tuple>

namespace FPMAS::agent {
	template<typename, typename...> class Agent;
}

namespace nlohmann {
	using FPMAS::agent::Agent;


	template<typename AgentTypeEnum, typename... Types>
    struct adl_serializer<std::unique_ptr<Agent<AgentTypeEnum, Types...>>> {
		static void to_json(json& j, const std::unique_ptr<Agent<AgentTypeEnum, Types...>>& data) {
			Agent<AgentTypeEnum, Types...>::to_json_t(j, data, Types()...);
		}

		static void from_json(const json& j, std::unique_ptr<Agent<AgentTypeEnum, Types...>>& agent_ptr) {
			AgentTypeEnum type = j.at("type").get<AgentTypeEnum>();
			Agent<AgentTypeEnum, Types...>::from_json_t(j, agent_ptr, type, Types()...);
			}
		};
}
#endif
