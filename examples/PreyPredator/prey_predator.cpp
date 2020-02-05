#include "prey_predator.h"

int Agent::currentRank() {
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	return rank;
}

const std::string Agent::getLabel() const {
	return label;
}

const Role Agent::getRole() const {
	return role;
}

const State Agent::getState() const {
	return state;
}

int Agent::getEnergy() const {
	return energy;
}

void Agent::eat(Agent& prey) {
	FPMAS_LOGW(currentRank(), "PREDATOR", "%s eats %s", this->getLabel().c_str(), prey.getLabel().c_str());

	prey.die();
	energy++;
}
void Agent::die() {
	FPMAS_LOGW(currentRank(), "PREY", "%s dies", this->getLabel().c_str());
	this->state=DEAD;
}
void Agent::predator_behavior(NEIGHBORS neighbors) {
	for(auto node : neighbors) {
		FPMAS_LOGI(currentRank(), "PREDATOR", "reading node %lu", node->getId());
		// Reads the neighbor data : instantaneous if local, block until
		// reception otherwise.
		Agent agent = node->data()->read();
		if(agent.role == PREY) {
			FPMAS_LOGW(currentRank(), "PREDATOR", "%s tries to eat %s!", label.c_str(), agent.label.c_str());
			// If the read agent is a PREY, we want to modify its internal
			// state so we need to acquire it
			Agent& prey = node->data()->acquire();
			// The state of the PREY must be read while data is ACQUIRED!
			// This ensures that no other predator can concurrently eat the prey
			if(prey.state == ALIVE) {
				// If its alive, this predator eats the prey.
				// If other predators were waiting to try to eat the prey, they
				// will receive an "eaten" prey when it is released from this
				// proc.
				this->eat(prey);
			} else {
				FPMAS_LOGW(currentRank(), "PREDATOR", "%s missed %s...", label.c_str(), prey.label.c_str());
			}
			// Releases the prey for other procs
			node->data()->release();
		}
	}
};


void Agent::act(NEIGHBORS perceived_neighbors) {
	FPMAS_LOGV(currentRank(), "AGENT", "%s acting", label.c_str());
	for(auto neighbor : perceived_neighbors) {
		switch(role) {
			case PREY:
				prey_behavior(perceived_neighbors);
				break;
			case PREDATOR:
				predator_behavior(perceived_neighbors);
				break;
		}
	}
};
