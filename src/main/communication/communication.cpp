#include "communication.h"

Communication::Communication() {
	MPI_Comm_rank(MPI_COMM_WORLD, &this->_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &this->_size);

	MPI_Comm_group(MPI_COMM_WORLD, &this->worldGroup);
}

int Communication::size() {
	return _size;
}

int Communication::rank() {
	return _rank;
}

MPI_Group Communication::getWorldGroup() {
	return this->worldGroup;
}
