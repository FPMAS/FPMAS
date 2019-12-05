#include "proxy.h"

using FPMAS::graph::proxy::Proxy;

/**
 * Sets the rank of the proc in which this proxy is currently living.
 *
 * @param localProc local proc rank
 */
void Proxy::setLocalProc(int localProc) {
	this->localProc = localProc;
}

/**
 * Sets the origin of the node corresponding to the specified id to proc.
 *
 * @param id node id
 * @param proc origin rank of the corresponding node
 */
void Proxy::setOrigin(unsigned long id, int proc) {
	this->origins[id] = proc;
}

/**
 * Returns the origin rank of the node corresponding to the specified id.
 *
 * The origin of node is actually used as the reference proxy from which we can
 * get the updated current location of the node.
 *
 * `localProc` is returned if the origin of the node is the current proc.
 *
 * @param id node id
 */
int Proxy::getOrigin(unsigned long id) {
	if(this->origins.count(id) == 1)
		return this->origins.at(id);

	return this->localProc;
}

/**
 * Sets the current location of the node corresponding to the specified id to
 * proc.
 *
 * @param id node id
 * @param proc current location rank of the corresponding node
 */
void Proxy::setCurrentLocation(unsigned long id, int proc) {
	this->currentLocations[id] = proc;
}

/**
 * Returns the current location of the node corresponding to the specified id.
 *
 * If the Proxy is properly synchronized, it is ensured that the node is
 * really living on the returned proc, what means that its associated data can
 * be asked directly to that proc.
 *
 * @param id node id
 */
int Proxy::getCurrentLocation(unsigned long id) {
	if(this->currentLocations.count(id) == 1)
		return this->currentLocations.at(id);
	return this->localProc;
}

void Proxy::setLocal(unsigned long id) {
	this->currentLocations.erase(id);
}
