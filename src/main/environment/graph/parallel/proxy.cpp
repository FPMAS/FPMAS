#include "proxy.h"

using FPMAS::graph::proxy::Proxy;

void Proxy::setLocalProc(int localProc) {
	this->localProc = localProc;
}

void Proxy::setOrigin(unsigned long id, int proc) {
	this->origins[id] = proc;
}

int Proxy::getOrigin(unsigned long id) {
	if(this->origins.count(id) == 1)
		return this->origins.at(id);

	return this->localProc;
}

void Proxy::setCurrentLocation(unsigned long id, int proc) {
	this->currentLocations[id] = proc;
}

int Proxy::getCurrentLocation(unsigned long id) {
	if(this->currentLocations.count(id) == 1)
		return this->currentLocations.at(id);
	return this->localProc;
}
