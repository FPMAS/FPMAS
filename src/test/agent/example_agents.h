#ifndef EXAMPLE_AGENTS_H
#define EXAMPLE_AGENTS_H

#include "graph/parallel/synchro/ghost_mode.h"
#include "agent/agent.h"

using FPMAS::graph::parallel::synchro::modes::GhostMode;
using FPMAS::agent::Agent;
using FPMAS::agent::DefaultCollector;

class Sheep;
class Wolf;

static const LayerId LAYER_A = 0;
static const LayerId LAYER_B = 1;

template<typename node_type> class LayerA : public DefaultCollector<node_type, LAYER_A> {
	public:
		LayerA(const node_type* node)
			: DefaultCollector<node_type, LAYER_A>(node) {};
};

template<typename node_type> class LayerB : public DefaultCollector<node_type, LAYER_B> {
	public:
		LayerB(const node_type* node)
			: DefaultCollector<node_type, LAYER_B>(node) {};
};

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
