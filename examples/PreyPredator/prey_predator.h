#ifndef PREY_PREDATOR_H
#define PREY_PREDATOR_H

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/synchro/hard_sync_mode.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::base::Node;
using FPMAS::graph::parallel::synchro::HardSyncMode;
using FPMAS::graph::parallel::synchro::SyncData;

enum Role {
	PREY,
	PREDATOR
};

enum State {
	ALIVE, DEAD
};

#define NEIGHBORS std::vector<Node<std::unique_ptr<SyncData<Agent>>, 1>*>


class Agent {
	private:
		// Attributes
		std::string label;
		Role role;
		State state = ALIVE;
		int energy = 0;

		// Actions
		void eat(Agent&);
		void die();

		// Behaviors
		void prey_behavior(NEIGHBORS) {};
		void predator_behavior(NEIGHBORS);


	public:
		// MPI Rank
		static int currentRank();

		// Constructors
		Agent() {};
		Agent(std::string label, Role role) : label(label), role(role) {}
		Agent(std::string label, Role role, State state, int energy) : label(label), role(role), state(state), energy(energy) {}

		// Getters
		const std::string getLabel() const;
		const Role getRole() const;
		const State getState() const;
		int getEnergy() const;

		// Action
		void act(NEIGHBORS perceived_neighbors);

};
#endif
