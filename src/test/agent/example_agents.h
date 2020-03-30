#ifndef EXAMPLE_AGENTS_H
#define EXAMPLE_AGENTS_H

#include "graph/parallel/synchro/ghost_mode.h"
#include "agent/agent.h"

using FPMAS::graph::parallel::synchro::modes::GhostMode;
using FPMAS::agent::Agent;

class Sheep;
class Wolf;

static const LayerId Layer_A = 0;
static const LayerId Layer_B = 1;

typedef Agent<GhostMode, Wolf, Sheep> PreyPredAgent;
typedef typename PreyPredAgent::node_ptr node_ptr;
typedef typename PreyPredAgent::env_type& env_ref;

class Wolf : public PreyPredAgent {
	public:
		void act(node_ptr, env_ref) override {}
};

template<>
struct nlohmann::adl_serializer<Wolf> {
	static void to_json(json& j, const Wolf& w) {
		j["role"] = "WOLF";
	}
	static Wolf from_json(const json&) {
		return Wolf();
	}
};

class Sheep : public PreyPredAgent {
	public:
		void act(node_ptr, env_ref) override {}
};

template<>
struct nlohmann::adl_serializer<Sheep> {
	static void to_json(json& j, const Sheep& w) {
		j["role"] = "SHEEP";
	}
	static Sheep from_json(const json&) { return Sheep();}
};
#endif
