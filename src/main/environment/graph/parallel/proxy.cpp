#include "proxy.h"

using FPMAS::graph::proxy::Location;
using FPMAS::graph::proxy::Proxy;

Location::Location() : Location(0, 0) {};

Location::Location(int proc, int part)
	: proc(proc), part(part) { }

void Proxy::setOrigin(unsigned long id, int proc, int part) {
	this->origins[id] = Location(proc, part);

	// TODO : THIS IS TEMPORARY
	this->currentLocations[id] = Location(proc, part);
}

Location Proxy::getOrigin(unsigned long id) {
	return this->origins.at(id);
}

Location Proxy::getCurrentLocation(unsigned long id) {
	return this->currentLocations.at(id);
}
