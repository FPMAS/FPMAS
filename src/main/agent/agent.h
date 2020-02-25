#ifndef AGENT_H
#define AGENT_H

#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
#include "serializer.h"

using nlohmann::json;

namespace FPMAS::agent {

	// functions to handle perceptions might be implemented
	template<typename AgentTypeEnum, typename... Types> class Agent {
		private:
			AgentTypeEnum type;
		public:
			Agent(AgentTypeEnum type) : type(type) {};
			AgentTypeEnum getType() const {return type;}

			virtual void act() = 0;

			template<typename T, typename... Ts>
				static void to_json_t(json& j, const std::unique_ptr<Agent<AgentTypeEnum, Types...>>& a, T type, Ts... types) {
					if(typeid(*a) == typeid(type)) {
						j["type"] = a->getType();
						nlohmann::adl_serializer<T>::to_json(j["agent"], dynamic_cast<T&>(*a));
						return;
					}
					to_json_t(j, a, types...);
				}
				static void to_json_t(json& j, const std::unique_ptr<Agent<AgentTypeEnum, Types...>>& a) {
				}


			template<typename T, typename... Ts>
				static void from_json_t(const json& j, std::unique_ptr<Agent<AgentTypeEnum, Types...>>& agent_ptr, AgentTypeEnum enum_type, T type, Ts... types) {
					if(enum_type == T::type) {
						T agent;
						nlohmann::adl_serializer<T>::from_json(j, agent);
						agent_ptr = std::unique_ptr<Agent<AgentTypeEnum, Types...>>(new T(agent));
						return;
					}
					from_json_t(j, agent_ptr, enum_type, types...);
				}
				static void from_json_t(const json& j, std::unique_ptr<Agent<AgentTypeEnum, Types...>>& agent_ptr, AgentTypeEnum enum_type) { }
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
