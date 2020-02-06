#include "utils/log.h"

#include "sync_communication.h"

#include "nlohmann/json.hpp"

using FPMAS::communication::SyncMpiCommunicator;
using FPMAS::communication::Tag;
/**
 * Builds a SyncMpiCommunicator containing all the procs available in
 * MPI_COMM_WORLD.
 *
 * @param resourceContainer pointer to the ResourceContainer instance that will be
 * used to query serialized data.
 */
SyncMpiCommunicator::SyncMpiCommunicator(ResourceContainer& resourceContainer)
	: resourceContainer(resourceContainer) {};

/**
 * Builds a SyncMpiCommunicator containing all the procs corresponding to
 * the provided ranks in MPI_COMM_WORLD.
 *
 * @param resourceContainer pointer to the ResourceContainer instance that will be
 * used to query serialized data.
 * @param ranks ranks to include in the group / communicator
 */
SyncMpiCommunicator::SyncMpiCommunicator(ResourceContainer& resourceContainer, std::initializer_list<int> ranks) : MpiCommunicator(ranks), resourceContainer(resourceContainer) {};

/**
 * Performs a reception cycle to handle.
 *
 * This function should not be used by the user, but is left public because it
 * might be useful for unit testing.
 *
 * A call to this function will receive and handle at most one of each of the
 * following requests types :
 * - read request
 * - acquire request
 * - given back data
 */
void SyncMpiCommunicator::handleIncomingRequests() {
	int flag;
	MPI_Status req_status;

	// Check read request
	MPI_Iprobe(MPI_ANY_SOURCE, Tag::READ, this->getMpiComm(), &flag, &req_status);
	if(flag > 0) {
		unsigned long id;
		MPI_Recv(&id, 1, MPI_UNSIGNED_LONG, req_status.MPI_SOURCE, Tag::READ, this->getMpiComm(), &req_status);
		FPMAS_LOGD(this->getRank(), "RECV", "read request %lu from %i", id, req_status.MPI_SOURCE);
		this->respondToRead(req_status.MPI_SOURCE, id);
	}

	// Check acquire request
	MPI_Iprobe(MPI_ANY_SOURCE, Tag::ACQUIRE, this->getMpiComm(), &flag, &req_status);
	if(flag > 0) {
		unsigned long id;
		MPI_Recv(&id, 1, MPI_UNSIGNED_LONG, req_status.MPI_SOURCE, Tag::ACQUIRE, this->getMpiComm(), &req_status);
		FPMAS_LOGD(this->getRank(), "RECV", "acquire request %lu from %i", id, req_status.MPI_SOURCE);
		this->respondToAcquire(req_status.MPI_SOURCE, id);
	}

	// Check acquire give back
	MPI_Iprobe(MPI_ANY_SOURCE, Tag::ACQUIRE_GIVE_BACK, this->getMpiComm(), &flag, &req_status);
	if(flag > 0) {
		int count;
		MPI_Get_count(&req_status, MPI_CHAR, &count);
		char data[count];
		MPI_Recv(&data, count, MPI_CHAR, req_status.MPI_SOURCE, Tag::ACQUIRE_GIVE_BACK, this->getMpiComm(), &req_status);

		std::string update_str (data);
		FPMAS_LOGD(this->getRank(), "RECV", "given back from %i : %s", req_status.MPI_SOURCE, update_str.c_str());
		nlohmann::json update_json = nlohmann::json::parse(update_str);
		unsigned long id = 
				update_json.at("id").get<unsigned long>();

		this->resourceContainer.updateData(
				id,
				update_json.at("data").get<std::string>()
				);

		this->resourceManager.releaseWrite(id);
	}
}

void SyncMpiCommunicator::waitSendRequest(MPI_Request* req) {
	MPI_Status send_status;
	int request_sent;
	MPI_Test(req, &request_sent, &send_status);

	while(request_sent == 0) {
		this->handleIncomingRequests();

		MPI_Test(req, &request_sent, &send_status);
	}
}

/**
 * Performs a synchronized read operation of the data corresponding to id on
 * the proc at the provided location, and returns the read serialized data.
 *
 * @param id id of the data to read
 * @param location rank of the data location
 * @return read serialized data
 */
std::string SyncMpiCommunicator::read(unsigned long id, int location) {
	if(location == this->getRank()) {
		FPMAS_LOGD(location, "READ", "reading local data %lu", id);
		// The local process needs to wait as any other proc to access its own
		// resources.
		this->waitForReading(id);

		std::string data = this->resourceContainer.getLocalData(id);

		this->resourceManager.releaseRead(id);
		return data;
	}
	FPMAS_LOGD(this->getRank(), "READ", "reading data %lu from %i", id, location);
	// Starts non-blocking synchronous send
	MPI_Request req;
	MPI_Issend(&id, 1, MPI_UNSIGNED_LONG, location, Tag::READ, this->getMpiComm(), &req);

	// Keep responding to other READ / ACQUIRE request to avoid deadlock,
	// until the request has been received
	this->waitSendRequest(&req);

	// The request has been received : it is assumed that the receiving proc is
	// now responding so we can safely wait for response without deadlocking
	MPI_Status read_response_status;
	MPI_Probe(location, Tag::READ_RESPONSE, this->getMpiComm(), &read_response_status);
	int count;
	MPI_Get_count(&read_response_status, MPI_CHAR, &count);
	char data[count];
	MPI_Recv(&data, count, MPI_CHAR, location, Tag::READ_RESPONSE, this->getMpiComm(), &read_response_status);

	return std::string(data);

}

void SyncMpiCommunicator::waitForReading(unsigned long id) {
	FPMAS_LOGV(this->getRank(), "READ", "wait for reading %lu", id);
	const ReadersWriters& rw = this->resourceManager.get(id);
	while(!rw.isAvailableForReading()) {
		this->handleIncomingRequests();
	}
	this->resourceManager.initRead(id);
	FPMAS_LOGV(this->getRank(), "READ", "reading %lu", id);
}

/*
 * Sends a read response to the destination proc, reading data using the
 * resourceManager.
 */
void SyncMpiCommunicator::respondToRead(int destination, unsigned long id) {
	if(this->resourceManager.isLocallyAcquired(id) > 0) {
		this->resourceManager.addPendingRead(id, destination);
		return;
	}
	this->color = Color::BLACK;

	// Wait for an eventual writing to id to finish
	this->waitForReading(id);

	// Perform the response
	std::string data = this->resourceContainer.getLocalData(id);
	MPI_Request req;
	// TODO : asynchronous send : is this a pb? => probably not.
	MPI_Isend(data.c_str(), data.length() + 1, MPI_CHAR, destination, Tag::READ_RESPONSE, this->getMpiComm(), &req);
	FPMAS_LOGD(this->getRank(), "READ", "send read data %lu to %i : %s", id, destination, data.c_str());

	// Release the resource in the resourceManager
	this->resourceManager.releaseRead(id);
}

/**
 * Performs a synchronized acquire operation of the data corresponding to id on
 * the proc at the provided location, and returns the acquired serialized data.
 *
 * @param id id of the data to read
 * @param location rank of the data location
 * @return acquired serialized data
 */
std::string SyncMpiCommunicator::acquire(unsigned long id, int location) {
	if(location == this->getRank()) {
		FPMAS_LOGD(location, "ACQUIRE", "acquiring local data %lu", id);
		// The local process needs to wait as any other proc to access its own
		// resources.
		this->waitForAcquire(id);
		this->resourceManager.locallyAcquired.insert(id);
		return this->resourceContainer.getLocalData(id);
	}
	FPMAS_LOGD(this->getRank(), "ACQUIRE", "acquiring data %lu from %i", id, location);
	// Starts non-blocking synchronous send
	MPI_Request req;
	MPI_Issend(&id, 1, MPI_UNSIGNED_LONG, location, Tag::ACQUIRE, this->getMpiComm(), &req);

	this->waitSendRequest(&req);

	// The request has been received : it is assumed that the receiving proc is
	// now responding so we can safely wait for response without deadlocking
	MPI_Status read_response_status;
	MPI_Probe(location, Tag::ACQUIRE_RESPONSE, this->getMpiComm(), &read_response_status);
	int count;
	MPI_Get_count(&read_response_status, MPI_CHAR, &count);
	char data[count];
	MPI_Recv(&data, count, MPI_CHAR, location, Tag::ACQUIRE_RESPONSE, this->getMpiComm(), &read_response_status);
	FPMAS_LOGD(this->getRank(), "ACQUIRE", "acquired data %lu from %i : %s", id, location, data);

	return std::string(data);
}

/**
 * Gives back a previously acquired data to location proc, sending potential
 * updates that occured while data was locally acquired.
 *
 * @param id id of the data to read
 * @param location rank of the data location
 */
void SyncMpiCommunicator::giveBack(unsigned long id, int location) {
	if(location == this->getRank()) {
		// No update needed, because modifications are already local.
		this->resourceManager.releaseWrite(id);
		this->resourceManager.locallyAcquired.erase(id);

		for(auto proc : this->resourceManager.pendingReads[id])
			this->respondToRead(proc, id);
		this->resourceManager.pendingReads.erase(id);

		for(auto proc : this->resourceManager.pendingAcquires[id])
			this->respondToAcquire(proc, id);
		this->resourceManager.pendingAcquires.erase(id);

		FPMAS_LOGD(this->getRank(), "ACQUIRE", "locally given back %lu", id);
		return;
	}

	// Perform the response
	nlohmann::json update = {
		{"id", id},
		{"data", this->resourceContainer.getUpdatedData(id)}
	};
	std::string data = update.dump();
	FPMAS_LOGD(this->getRank(), "ACQUIRE", "giving back to %i : %s", location, data.c_str());
	MPI_Request req;
	// TODO : asynchronous send : is this a pb? => probably not.
	MPI_Isend(data.c_str(), data.length() + 1, MPI_CHAR, location, Tag::ACQUIRE_GIVE_BACK, this->getMpiComm(), &req);
}

void SyncMpiCommunicator::waitForAcquire(unsigned long id) {
	FPMAS_LOGV(this->getRank(), "ACQUIRE", "wait for acquiring %lu", id);
	const ReadersWriters& rw = this->resourceManager.get(id);
	while(!rw.isAvailableForWriting()) {
		this->handleIncomingRequests();
	}
	FPMAS_LOGV(this->getRank(), "ACQUIRE", "acquired %lu", id);
	this->resourceManager.initWrite(id);
}

void SyncMpiCommunicator::respondToAcquire(int destination, unsigned long id) {
	if(this->resourceManager.isLocallyAcquired(id) > 0) {
		this->resourceManager.addPendingAcquire(id, destination);
		return;
	}
	this->color = Color::BLACK;

	this->waitForAcquire(id);
	std::string data = this->resourceContainer.getLocalData(id);
	FPMAS_LOGD(this->getRank(), "ACQUIRE", "send acquired data %lu to %i : %s", id, destination, data.c_str());

	MPI_Request req;
	// TODO : asynchronous send : is this a pb? => probably not.
	MPI_Issend(data.c_str(), data.length() + 1, MPI_CHAR, destination, Tag::ACQUIRE_RESPONSE, this->getMpiComm(), &req);

	this->waitSendRequest(&req);
}

/**
 * Implements the Dijkstra-Feijen-Gasteren termination algorithm.
 *
 * This function must be called when the process using this communicator is
 * done with read and acquire operations to other procs. When terminate() is
 * called, it will enter a PASSIVE mode where it is only able to respond to
 * other procs READ and ACQUIRE requests (ie ACTIVE processes that have not
 * terminated yet).
 *
 * Returns only when all the processes have terminated their process.
 */
void SyncMpiCommunicator::terminate() {
	this->state = State::PASSIVE;
	bool end = false;
	int token;

	if(this->getRank() == 0) {
		this->color = Color::WHITE;
		token = Color::WHITE;
		MPI_Send(&token, 1, MPI_INT, this->getSize() - 1, Tag::TOKEN, this->getMpiComm());
	}

	int flag;
	MPI_Status status;
	while(!end) {
		// Check for TOKEN
		MPI_Iprobe(MPI_ANY_SOURCE, Tag::TOKEN, this->getMpiComm(), &flag, &status);
		if(flag > 0) {
			MPI_Recv(&token, 1, MPI_INT, status.MPI_SOURCE, TOKEN, this->getMpiComm(), &status);
			if(this->getRank() == 0) {
				if(token == Color::WHITE && this->color == Color::WHITE) {
					end = true;
					for (int i = 1; i < this->getSize(); ++i) {
						MPI_Ssend(NULL, 0, MPI_INT, i, Tag::END, this->getMpiComm());	
					}
				} else {
					this->color = Color::WHITE;
					token = Color::WHITE;
					MPI_Send(&token, 1, MPI_INT, this->getSize() - 1, Tag::TOKEN, this->getMpiComm());
				}
			}
			else {
				if(this->color == Color::BLACK) {
					token = Color::BLACK;
				}
				MPI_Send(&token, 1, MPI_INT, this->getRank() - 1, Tag::TOKEN, this->getMpiComm());
				this->color = Color::WHITE;
			}
		}

		// Check for END
		MPI_Iprobe(MPI_ANY_SOURCE, Tag::END, this->getMpiComm(), &flag, &status);
		if(flag > 0) {
			MPI_Recv(NULL, 0, MPI_INT, status.MPI_SOURCE, Tag::END, this->getMpiComm(), &status);
			end = true;
		}

		// Handles READ or ACQUIRE requests
		this->handleIncomingRequests();
	}
	this->resourceManager.clear();
}
