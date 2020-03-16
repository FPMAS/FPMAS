#ifndef AGENT_SERIALIZER_H
#define AGENT_SERIALIZER_H

#include <nlohmann/json.hpp>
#include <typeindex>
#include <tuple>

namespace FPMAS::agent {
	template<typename...> class Agent;
}

namespace nlohmann {
	using FPMAS::agent::Agent;

	template<typename... Types>
    struct adl_serializer<std::unique_ptr<Agent<Types...>>> {
		static const std::unordered_map<std::type_index, unsigned long> type_id_map;
		static const std::unordered_map<unsigned long, std::type_index> id_type_map;

		static std::unordered_map<std::type_index, unsigned long> init_type_id_map() {
			int i = 0;
			std::unordered_map<std::type_index, unsigned long> map;
			(map.insert(std::make_pair(std::type_index(typeid(Types)), i++)), ...);
			return map;
		}

		static std::unordered_map<unsigned long, std::type_index> init_id_type_map() {
			int i = 0;
			std::unordered_map<unsigned long, std::type_index> map;
			(map.insert(std::make_pair(i++, std::type_index(typeid(Types)))), ...);
			return map;
		}

		static void to_json(json& j, const std::unique_ptr<Agent<Types...>>& data) {
			Agent<Types...>::to_json_t(j, data, Types()...);
		}

		static void from_json(const json& j, std::unique_ptr<Agent<Types...>>& agent_ptr) {
			Agent<Types...>::from_json_t(j, agent_ptr, j.at("type").get<unsigned long>(), Types()...);
			}
		};
	template<typename... Types>
		const std::unordered_map<std::type_index, unsigned long> 
		adl_serializer<std::unique_ptr<Agent<Types...>>>::type_id_map
			= init_type_id_map();
	template<typename... Types>
		const std::unordered_map<unsigned long, std::type_index> 
		adl_serializer<std::unique_ptr<Agent<Types...>>>::id_type_map
			= init_id_type_map();

}
#endif
