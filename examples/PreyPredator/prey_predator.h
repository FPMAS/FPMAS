#ifndef PREY_PREDATOR_H
#define PREY_PREDATOR_H

#include "graph/parallel/distributed_graph.h"
#include "graph/parallel/synchro/hard_sync_data.h"

using FPMAS::graph::parallel::DistributedGraph;
using FPMAS::graph::base::Node;
using FPMAS::graph::parallel::synchro::HardSyncData;
using FPMAS::graph::parallel::synchro::SyncDataPtr;

enum Role {
	PREY,
	PREDATOR
};

enum State {
	ALIVE, DEAD
};

#define NEIGHBORS std::vector<Node<SyncDataPtr<Agent>>*>


class Agent {
	private:
		std::string label;
		Role role;
		State state = ALIVE;
		int energy = 0;
		void eat(Agent&);
		void die();
		void prey_behavior(NEIGHBORS) {};
		void predator_behavior(NEIGHBORS);


	public:
		static int currentRank() {
			int rank;
			MPI_Comm_rank(MPI_COMM_WORLD, &rank);
			return rank;
		}
		Agent() {};
		Agent(std::string label, Role role) : label(label), role(role) {}
		Agent(std::string label, Role role, State state, int energy) : label(label), role(role), state(state), energy(energy) {}
		void act(NEIGHBORS perceived_neighbors);
		const std::string getLabel() const;
		const Role getRole() const;
		const State getState() const;
		int getEnergy() const;

};
#endif
