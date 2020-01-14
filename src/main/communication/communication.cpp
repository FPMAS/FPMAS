#include "communication.h"
#include <cstdarg>

using FPMAS::communication::MpiCommunicator;
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
}

/**
 * Returns the built MPI communicator.
 *
 * @return associated MPI communicator
 */
MPI_Comm MpiCommunicator::getMpiComm() {
	return this->comm;
}

/**
 * Returns the built MPI group.
 *
 * @return associated MPI group
 */
MPI_Group MpiCommunicator::getMpiGroup() {
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

void send(std::string message, int destination, Tag tag) {

}

void MpiCommunicator::terminate() {
	this->state = State::PASSIVE;
	bool end = false;
	int token;

	if(this->rank == 0) {
		this->color = Color::WHITE;
		token = Color::WHITE;
		MPI_Send(&token, 1, MPI_INT, this->size - 1, Tag::TOKEN, this->comm);
	}

	MPI_Status status;
	while(!end) {
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, this->comm, &status);
		int tag;

		switch(status.MPI_TAG) {
			case TOKEN:
				MPI_Recv(&token, 1, MPI_INT, status.MPI_SOURCE, TOKEN, this->comm, &status);
				if(this->rank == 0) {
					if(token == Color::WHITE && this->color == Color::WHITE) {
						end = true;
						for (int i = 1; i < this->size; ++i) {
							MPI_Ssend(NULL, 0, MPI_INT, i, Tag::END, this->comm);	
						}
					} else {
						this->color = Color::WHITE;
						token = Color::WHITE;
						MPI_Send(&token, 1, MPI_INT, this->size - 1, Tag::TOKEN, this->comm);
					}
				}
				else {
					if(this->color == Color::BLACK) {
						token = Color::BLACK;
					}
					MPI_Send(&token, 1, MPI_INT, this->rank - 1, Tag::TOKEN, this->comm);
					this->color = Color::WHITE;
				}
				break;
			
			case END:
				MPI_Recv(NULL, 0, MPI_INT, status.MPI_SOURCE, Tag::END, this->comm, &status);
				end = true;
				break;

			default:
				int count;
				MPI_Get_count(&status, MPI_INT, &count);
				this->state = State::ACTIVE;
				
				// Handle ACQUIRE and READ messages
				// If send response, become active and black color
				break;
		}

	}

}
