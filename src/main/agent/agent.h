#ifndef AGENT_H
#define AGENT_H

#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
#include "serializer.h"

using nlohmann::json;

namespace FPMAS::agent {

	// functions to handle perceptions might be implemented
	template<typename... Types> class Agent {
		private:
		typedef nlohmann::adl_serializer<std::unique_ptr<Agent<Types...>>>
			AgentSerializer;
		public:
			virtual void act() = 0;

			template<typename T, typename... Ts>
				static void to_json_t(json& j, const std::unique_ptr<Agent<Types...>>& a, T type, Ts... types) {
					if(typeid(*a) == typeid(type)) {
						j["type"] = AgentSerializer::type_id_map.at(typeid(type));
						nlohmann::adl_serializer<T>::to_json(j["agent"], dynamic_cast<T&>(*a));
						return;
					}
					to_json_t(j, a, types...);
				}
				static void to_json_t(json& j, const std::unique_ptr<Agent<Types...>>& a) {
				}


			template<typename T, typename... Ts>
				static void from_json_t(const json& j, std::unique_ptr<Agent<Types...>>& agent_ptr, unsigned long type_id, T type, Ts... types) {
					const nlohmann::TypeInfoRef type_info = (nlohmann::TypeInfoRef) typeid(type);
					if(type_info.get() == AgentSerializer::id_type_map.at(type_id).get()) {
						T agent;
						nlohmann::adl_serializer<T>::from_json(j, agent);
						agent_ptr = std::unique_ptr<Agent<Types...>>(new T(agent));
						return;
					}
					from_json_t(j, agent_ptr, type_id, types...);
				}
				static void from_json_t(const json& j, std::unique_ptr<Agent<Types...>>& agent_ptr, unsigned long type_id) { }
	};

}
/*
 *
 *    class AgentPtr {
 *        Agent* agent;
 *
 *        public:
 *            AgentPtr(Agent*);
 *            AgentPtr(const AgentPtr&) = delete;
 *            AgentPtr& operator=(const AgentPtr&) = delete;
 *
 *            AgentPtr(AgentPtr&&);
 *            AgentPtr& operator=(AgentPtr&&);
 *
 *            ~AgentPtr();
 *
 *    };
 */

#endif
