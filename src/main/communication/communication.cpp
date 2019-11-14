#include "communication.h"

using FPMAS::communication::MpiCommunicator;

MpiCommunicator::MpiCommunicator() {
	MPI_Group worldGroup;
	MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
	MPI_Group_union(worldGroup, MPI_GROUP_EMPTY, &this->group);
	MPI_Comm_create(MPI_COMM_WORLD, this->group, &this->comm);

	MPI_Comm_rank(this->comm, &this->rank);
	MPI_Comm_size(this->comm, &this->size);
}

MPI_Comm MpiCommunicator::getMpiComm() {
	return this->comm;
}

MPI_Group MpiCommunicator::getMpiGroup() {
	return this->group;
}

int MpiCommunicator::getRank() {
	return this->rank;
}

int MpiCommunicator::getSize() {
	return this->size;
}
