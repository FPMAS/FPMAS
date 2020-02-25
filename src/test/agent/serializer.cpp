#include "gtest/gtest.h"
#include "agent/agent.h"

using FPMAS::agent::Agent;

enum AgentTypes {
	WOLF,
	SHEEP
};

class Sheep;
class Wolf;

typedef Agent<AgentTypes, Wolf, Sheep> PreyPredAgent;

class Wolf : public PreyPredAgent {
	public:
		static const AgentTypes type = WOLF;
		Wolf() : PreyPredAgent(WOLF){};

		void act() {}
};
void to_json(json& j, const Wolf& w) {
	j["role"] = "WOLF";
}
void from_json(const json&, Wolf&) {}

class Sheep : public PreyPredAgent {
	public:
		static const AgentTypes type = SHEEP;
		Sheep() : PreyPredAgent(SHEEP){};

		void act() {}
};

void to_json(json& j, const Sheep& w) {
	j["role"] = "SHEEP";
}
void from_json(const json&, Sheep&) {}

TEST(SerializerTest, serialize) {
	std::unique_ptr<PreyPredAgent> wolf_ptr(new Wolf());
	std::unique_ptr<PreyPredAgent> sheep_ptr(new Sheep());

	json j_wolf = wolf_ptr;
	json j_sheep = sheep_ptr;

	ASSERT_EQ(j_wolf.size(), 2);
	ASSERT_EQ(j_wolf.at("type").get<AgentTypes>(), WOLF);
	ASSERT_EQ(j_wolf.at("agent").at("role").get<std::string>(), "WOLF");
	
	ASSERT_EQ(j_sheep.size(), 2);
	ASSERT_EQ(j_sheep.at("type").get<AgentTypes>(), SHEEP);
	ASSERT_EQ(j_sheep.at("agent").at("role").get<std::string>(), "SHEEP");
}
