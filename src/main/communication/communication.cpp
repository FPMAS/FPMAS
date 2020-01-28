#include "communication.h"
#include <nlohmann/json.hpp>
#include <cstdarg>

using FPMAS::communication::MpiCommunicator;
using FPMAS::communication::TerminableMpiCommunicator;
using FPMAS::communication::Tag;

/**
 * Builds an MPI group and the associated communicator as a copy of the
 * MPI_COMM_WORLD communicator.
 */
MpiCommunicator::MpiCommunicator() {
	MPI_Group worldGroup;
	MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
	MPI_Group_union(worldGroup, MPI_GROUP_EMPTY, &this->group);
	MPI_Comm_create(MPI_COMM_WORLD, this->group, &this->comm);

	MPI_Comm_rank(this->comm, &this->rank);
	MPI_Comm_size(this->comm, &this->size);

	MPI_Group_free(&worldGroup);
}

/**
 * Builds an MPI group and the associated communicator that contains the ranks
 * specified as arguments.
 *
 * ```cpp
 * MpiCommunicator comm {0, 1, 2};
 * ```
 *
 * Notice that such calls must follow the same requirements as the
 * `MPI_Comm_create` function. In consequence, **all the current procs** must
 * perform a call to this function to be valid.
 *
 * ```cpp
 * int current_ranks;
 * MPI_Comm_rank(&current_rank);
 *
 * if(current_rank < 3) {
 * 	MpiCommunicator comm {0, 1, 2};
 * } else {
 * 	MpiCommunicator comm {current_rank};
 * }
 * ```
 *
 * @param ranks ranks to include in the group / communicator
 */
MpiCommunicator::MpiCommunicator(std::initializer_list<int> ranks) {
	int _ranks[ranks.size()];
	int i = 0;
	for(auto rank : ranks) {
		_ranks[i++] = rank;
	}

	MPI_Group worldGroup;
	MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
	MPI_Group_incl(worldGroup, ranks.size(), _ranks, &this->group);
	MPI_Comm_create(MPI_COMM_WORLD, this->group, &this->comm);

	MPI_Comm_rank(this->comm, &this->rank);
	MPI_Comm_size(this->comm, &this->size);

	MPI_Group_free(&worldGroup);
}

/**
 * MpiCommunicator copy constructor.
 *
 * Allows a correct MPI resources management.
 *
 * @param from MpiCommunicator to copy from
 */
MpiCommunicator::MpiCommunicator(const MpiCommunicator& from) {
	this->comm = from.comm;
	this->comm_references = from.comm_references + 1;

	MPI_Comm_group(this->comm, &this->group);

	MPI_Comm_rank(this->comm, &this->rank);
	MPI_Comm_size(this->comm, &this->size);

}
/**
 * MpiCommunicator copy assignment.
 *
 * Allows a correct MPI resources management.
 *
 * @param other MpiCommunicator to assign to this
 */
MpiCommunicator& MpiCommunicator::operator=(MpiCommunicator other) {
	MPI_Group_free(&this->group);

	this->comm = other.comm;
	this->comm_references = other.comm_references;

	MPI_Comm_group(this->comm, &this->group);

	MPI_Comm_rank(this->comm, &this->rank);
	MPI_Comm_size(this->comm, &this->size);


	return *this;
}

/**
 * Returns the built MPI communicator.
 *
 * @return associated MPI communicator
 */
MPI_Comm MpiCommunicator::getMpiComm() const {
	return this->comm;
}

/**
 * Returns the built MPI group.
 *
 * @return associated MPI group
 */
MPI_Group MpiCommunicator::getMpiGroup() const {
	return this->group;
}

/**
 * Returns the rank of this communicator, in the meaning of MPI communicator
 * ranks.
 *
 * @return MPI communicator rank
 */
int MpiCommunicator::getRank() const {
	return this->rank;
}

/**
 * Returns the rank of this communicator, in the meaning of MPI communicator
 * sizes.
 *
 * @return MPI communicator size
 */
int MpiCommunicator::getSize() const {
	return this->size;
}

/**
 * MpiCommunicator destructor.
 *
 * Properly releases acquired MPI resources.
 */
MpiCommunicator::~MpiCommunicator() {
	MPI_Group_free(&this->group);
	if(this->comm_references == 1)
		MPI_Comm_free(&this->comm);
}

/**
 * Builds a TerminableMpiCommunicator containing all the procs available in
 * MPI_COMM_WORLD.
 *
 * @param resourceManager pointer to the ResourceManager instance that will be
 * used to query serialized data.
 */
TerminableMpiCommunicator::TerminableMpiCommunicator(ResourceContainer& resourceContainer)
	: resourceContainer(resourceContainer) {};

/**
 * Builds a TerminableMpiCommunicator containing all the procs corresponding to
 * the provided ranks in MPI_COMM_WORLD.
 *
 * @param resourceManager pointer to the ResourceManager instance that will be
 * used to query serialized data.
 * @param ranks ranks to include in the group / communicator
 */
TerminableMpiCommunicator::TerminableMpiCommunicator(ResourceContainer& resourceContainer, std::initializer_list<int> ranks) : MpiCommunicator(ranks), resourceContainer(resourceContainer) {};

void TerminableMpiCommunicator::handleIncomingRequests() {
	int flag;
	MPI_Status req_status;

	// Check read request
	MPI_Iprobe(MPI_ANY_SOURCE, Tag::READ, this->getMpiComm(), &flag, &req_status);
	if(flag > 0) {
		unsigned long id;
		MPI_Recv(&id, 1, MPI_UNSIGNED_LONG, req_status.MPI_SOURCE, Tag::READ, this->getMpiComm(), &req_status);
		std::cout << "[" << this->getRank() << "] read req : " << id << std::endl;
		this->respondToRead(req_status.MPI_SOURCE, id);
	}

	// Check acquire request
	MPI_Iprobe(MPI_ANY_SOURCE, Tag::ACQUIRE, this->getMpiComm(), &flag, &req_status);
	if(flag > 0) {
		unsigned long id;
		MPI_Recv(&id, 1, MPI_UNSIGNED_LONG, req_status.MPI_SOURCE, Tag::ACQUIRE, this->getMpiComm(), &req_status);
		std::cout << "[" << this->getRank() << "] acquire req : " << id << std::endl;
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
		std::cout << "[" << this->getRank() << "] given back : " << update_str << std::endl;
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

void TerminableMpiCommunicator::waitSendRequest(MPI_Request* req) {
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
std::string TerminableMpiCommunicator::read(unsigned long id, int location) {
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

void TerminableMpiCommunicator::waitForReading(unsigned long id) {
	const ReadersWriters& rw = this->resourceManager.get(id);
	while(!rw.isAvailableForReading()) {
		this->handleIncomingRequests();
	}
}

/*
 * Sends a read response to the destination proc, reading data using the
 * resourceManager.
 */
void TerminableMpiCommunicator::respondToRead(int destination, unsigned long id) {
	this->color = Color::BLACK;

	// Initialize the read request in the resourceManager
	this->resourceManager.initRead(id);

	// Wait for an eventual writing to id to finish
	this->waitForReading(id);

	// Perform the response
	std::string data = this->resourceContainer.getData(id);
	MPI_Request req;
	// TODO : asynchronous send : is this a pb? => probably not.
	MPI_Isend(data.c_str(), data.length() + 1, MPI_CHAR, destination, Tag::READ_RESPONSE, this->getMpiComm(), &req);

	// Release the resource in the resourceManager
	this->resourceManager.releaseRead(id);
}

std::string TerminableMpiCommunicator::acquire(unsigned long id, int location) {
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

	return std::string(data);
}

void TerminableMpiCommunicator::giveBack(unsigned long id, int location) {
	// Perform the response
	nlohmann::json update = {
		{"id", id},
		{"data", this->resourceContainer.getData(id)}
	};
	std::string data = update.dump();
	MPI_Request req;
	// TODO : asynchronous send : is this a pb? => probably not.
	MPI_Isend(data.c_str(), data.length() + 1, MPI_CHAR, location, Tag::ACQUIRE_GIVE_BACK, this->getMpiComm(), &req);
}

void TerminableMpiCommunicator::waitForAcquire(unsigned long id) {
	const ReadersWriters& rw = this->resourceManager.get(id);
	while(!rw.isAvailableForWriting()) {
		this->handleIncomingRequests();
	}
}

void TerminableMpiCommunicator::respondToAcquire(int destination, unsigned long id) {
	this->color = Color::BLACK;

	this->resourceManager.initWrite(id);

	std::cout << "[" << this->getRank() << "] wait acquire " << id << " for " << destination << std::endl;
	this->waitForAcquire(id);
	std::cout << "[" << this->getRank() << "] respond acquire ready" << std::endl;

	std::string data = this->resourceContainer.getData(id);
	std::cout << "[" << this->getRank() << "] respond data : " << data << std::endl;

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
void TerminableMpiCommunicator::terminate() {
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
}
