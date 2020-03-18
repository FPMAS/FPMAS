#ifndef AGENT_H
#define AGENT_H

#include <memory>
#include <iostream>
#include <nlohmann/json.hpp>
#include "graph/base/graph.h"
#include "environment/environment.h"
#include "serializer.h"

using nlohmann::json;

using FPMAS::graph::base::Node;
using FPMAS::graph::parallel::synchro::wrappers::SyncData;
using FPMAS::environment::Environment;

namespace FPMAS::agent {

	// functions to handle perceptions might be implemented
	template<SYNC_MODE, int N, typename... Types> class Agent {
		protected:
			typedef Agent<S, N, Types...> agent_type;

		private:
			typedef nlohmann::adl_serializer<std::unique_ptr<Agent<S, N, Types...>>>
				agent_serializer;
			friend agent_serializer;

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
					const std::unique_ptr<agent_type>& a,
					AgentType<T> type,
					AgentType<Ts>... types) {
					if(std::type_index(typeid(*a)) == type.type) {
						j["type"] = agent_serializer::type_id_map.at(type.type);
						nlohmann::adl_serializer<T>::to_json(j["agent"], dynamic_cast<T&>(*a));
						return;
					}
					to_json_t(j, a, types...);
				}
				static void to_json_t(json& j, const std::unique_ptr<agent_type>& a) {
					// Recursion base case
				}


			template<typename T, typename... Ts>
				static void from_json_t(
					const json& j,
					std::unique_ptr<agent_type>& agent_ptr,
					unsigned long type_id,
					AgentType<T> type,
					AgentType<Ts>... types) {
					if(std::type_index(typeid(type)) == agent_serializer::id_type_map.at(type_id)) {
						agent_ptr = std::unique_ptr<agent_type>(
							new T(nlohmann::adl_serializer<T>::from_json(j))
							);
						return;
					}
					from_json_t(j, agent_ptr, type_id, types...);
				}
				static void from_json_t(
					const json& j,
					std::unique_ptr<agent_type>& agent_ptr,
					unsigned long type_id) {
					// Recursion base case
				}
	};
}

#endif
