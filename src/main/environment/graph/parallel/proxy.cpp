#include "proxy.h"

using FPMAS::graph::proxy::Proxy;

void Proxy::setLocalProc(int localProc) {
	this->localProc = localProc;
}

void Proxy::setOrigin(unsigned long id, int proc) {
	this->origins[id] = proc;

	// TODO : THIS IS TEMPORARY
	this->currentLocations[id] = proc;
}

int Proxy::getOrigin(unsigned long id) {
	if(this->origins.count(id) == 1)
		return this->origins.at(id);

	return this->localProc;
}

int Proxy::getCurrentLocation(unsigned long id) {
	return this->currentLocations.at(id);
}
