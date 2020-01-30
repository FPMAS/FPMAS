#include "prey_predator.h"

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
		if(node->data()->read().role == PREY) {
			Agent& neighbor = node->data()->acquire();
			FPMAS_LOGW(currentRank(), "PREDATOR", "%s tries to eat %s!", label.c_str(), neighbor.label.c_str());
			if(neighbor.state == ALIVE) {
				this->eat(neighbor);
			} else {
				FPMAS_LOGW(currentRank(), "PREDATOR", "%s missed %s...", label.c_str(), neighbor.label.c_str());

			}
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
