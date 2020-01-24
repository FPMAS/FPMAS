#include "communication.h"
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
TerminableMpiCommunicator::TerminableMpiCommunicator(ResourceManager* resourceManager)
	: resourceManager(resourceManager) {};

/**
 * Builds a TerminableMpiCommunicator containing all the procs corresponding to
 * the provided ranks in MPI_COMM_WORLD.
 *
 * @param resourceManager pointer to the ResourceManager instance that will be
 * used to query serialized data.
 * @param ranks ranks to include in the group / communicator
 */
TerminableMpiCommunicator::TerminableMpiCommunicator(ResourceManager* resourceManager, std::initializer_list<int> ranks) : MpiCommunicator(ranks), resourceManager(resourceManager) {};

/**
 * Performs a synchronized read operation of the data corresponding to id on
 * the proc at the provided location, and returns the read serialized data.
 *
 * @param id id of the data to read
 * @param location rank of the data location
 * @return read serialized data
 */
std::string TerminableMpiCommunicator::read(unsigned long id, int location) {
	MPI_Request req;
	std::cout << "\e[32m[" << this->getRank() << "] ask " << id << " to " << location << "\e[0m" << std::endl;
	MPI_Issend(&id, 1, MPI_UNSIGNED_LONG, location, Tag::READ, this->getMpiComm(), &req);

	int request_sent;
	MPI_Status request_status;
	MPI_Test(&req, &request_sent, &request_status);

	int read_flag;
	MPI_Status read_status;
	while(request_sent == 0) {
		MPI_Iprobe(MPI_ANY_SOURCE, Tag::READ, this->getMpiComm(), &read_flag, &read_status);
		if(read_flag > 0) {
			unsigned long id;
			std::cout << "\e[32m[" << this->getRank() << "] rcv req\e[0m" << std::endl;
			MPI_Recv(&id, 1, MPI_UNSIGNED_LONG, read_status.MPI_SOURCE, Tag::READ, this->getMpiComm(), &read_status);
			this->respondToRead(read_status.MPI_SOURCE, id);
		}
		MPI_Test(&req, &request_sent, &request_status);
	}

	MPI_Probe(location, Tag::READ, this->getMpiComm(), &request_status);
	int count;
	MPI_Get_count(&request_status, MPI_CHAR, &count);
	char data[count];
	MPI_Recv(&data, count, MPI_CHAR, location, Tag::READ, this->getMpiComm(), &request_status);

	std::cout << "\e[32m[" << this->getRank() << "] rcv " << id << " : " << data << "\e[0m" << std::endl;
	return std::string(data);

}

/*
 * Sends a read response to the destination proc, reading data using the
 * resourceManager.
 */
void TerminableMpiCommunicator::respondToRead(int destination, unsigned long id) {
	this->color = Color::BLACK;
	std::string data = this->resourceManager->getResource(id);
	std::cout << "\e[32m[" << this->getRank() << "] send " << id << " : " << data << "\e[0m" << std::endl;
	MPI_Request req;
	// TODO : asynchronous send : is this a pb?
	MPI_Isend(data.c_str(), data.length() + 1, MPI_CHAR, destination, Tag::READ, this->getMpiComm(), &req);
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

	MPI_Status status;
	while(!end) {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, this->getMpiComm(), &status);
		int tag;

		switch(status.MPI_TAG) {
			case TOKEN:
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
				break;
			
			case END:
				MPI_Recv(NULL, 0, MPI_INT, status.MPI_SOURCE, Tag::END, this->getMpiComm(), &status);
				end = true;
				break;

			case READ:
				this->state = State::ACTIVE;

				int id;
				int count;
				MPI_Get_count(&status, MPI_INT, &count);

				MPI_Recv(&id, count, MPI_UNSIGNED_LONG, status.MPI_SOURCE, Tag::READ, this->getMpiComm(), &status);

				this->respondToRead(status.MPI_SOURCE, id);
				break;

			default:
				
				// Handle ACQUIRE and READ messages
				// If send response, become active and black color
				break;
		}

	}
}
