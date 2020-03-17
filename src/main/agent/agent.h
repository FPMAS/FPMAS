#ifndef AGENT_H
#define AGENT_H

#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
#include "serializer.h"

using nlohmann::json;

namespace FPMAS::agent {

	// functions to handle perceptions might be implemented
	template<int N, typename... Types> class Agent {
		private:
		typedef nlohmann::adl_serializer<std::unique_ptr<Agent<N, Types...>>>
			AgentSerializer;
		public:
			virtual void act() = 0;

			/*
			 * Even if they are useless at runtime, `type` and `types` must be
			 * provided, in order to allow the compiler to deduce T and Ts
			 * template arguments and perform a "template recursion".
			 *
			 * In consequence, T and Ts types must be DefaultConstructible.
			 */
			template<typename T, typename... Ts>
				static void to_json_t(
					json& j,
					const std::unique_ptr<Agent<N, Types...>>& a,
					T type,
					Ts... types) {
					if(typeid(*a) == typeid(type)) {
						j["type"] = AgentSerializer::type_id_map.at(typeid(type));
						nlohmann::adl_serializer<T>::to_json(j["agent"], dynamic_cast<T&>(*a));
						return;
					}
					to_json_t(j, a, types...);
				}
				static void to_json_t(json& j, const std::unique_ptr<Agent<N, Types...>>& a) {
					// Recursion base case
				}


			template<typename T, typename... Ts>
				static void from_json_t(
					const json& j,
					std::unique_ptr<Agent<N, Types...>>& agent_ptr,
					unsigned long type_id,
					T type,
					Ts... types) {
					const std::type_index type_index
						= std::type_index(typeid(type));
					if(type_index == AgentSerializer::id_type_map.at(type_id)) {
						agent_ptr = std::unique_ptr<Agent<N, Types...>>(
							new T(nlohmann::adl_serializer<T>::from_json(j))
							);
						return;
					}
					from_json_t(j, agent_ptr, type_id, types...);
				}
				static void from_json_t(
					const json& j,
					std::unique_ptr<Agent<N, Types...>>& agent_ptr,
					unsigned long type_id) {
					// Recursion base case
				}
	};

}

#endif
