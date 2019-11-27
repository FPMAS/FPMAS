#include "proxy.h"

using FPMAS::graph::proxy::Proxy;

void Proxy::setOrigin(unsigned long id, int proc) {
	this->origins[id] = proc;

	// TODO : THIS IS TEMPORARY
	this->currentLocations[id] = proc;
}

int Proxy::getOrigin(unsigned long id) {
	return this->origins.at(id);
}

int Proxy::getCurrentLocation(unsigned long id) {
	return this->currentLocations.at(id);
}
