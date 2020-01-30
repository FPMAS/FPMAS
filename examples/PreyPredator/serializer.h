#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "prey_predator.h"
#include "nlohmann/json.hpp"

NLOHMANN_JSON_SERIALIZE_ENUM( Role, {
    {PREY, "PREY"},
    {PREDATOR, "PREDATOR"}
})

NLOHMANN_JSON_SERIALIZE_ENUM( State, {
    {DEAD, "DEAD"},
    {ALIVE, "ALIVE"}
})

namespace nlohmann {
	template<>
    struct adl_serializer<Agent> {
		static Agent from_json(const json& j) {
			Agent agent = Agent(
					j.at("l").get<std::string>(),
					j.at("r").get<Role>(),
					j.at("s").get<State>(),
					j.at("e").get<int>()
					);
			return agent;
		}
		static void to_json(json& j, const Agent& agent) {
			j = json{
				{"l", agent.getLabel()},
				{"r", agent.getRole()},
				{"s", agent.getState()},
				{"e", agent.getEnergy()}
			};
		}

	};
}
#endif
