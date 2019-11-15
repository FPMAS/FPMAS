#include "communication.h"

using FPMAS::communication::MpiCommunicator;

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
int MpiCommunicator::getRank() {
	return this->rank;
}

/**
 * Returns the rank of this communicator, in the meaning of MPI communicator
 * sizes.
 *
 * @return MPI communicator size
 */
int MpiCommunicator::getSize() {
	return this->size;
}
