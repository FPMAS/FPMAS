#include "communication.h"
#include "mpi.h"

Communication::Communication() {
	MPI_Comm_rank(MPI_COMM_WORLD, &this->_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &this->_size);
}

int Communication::size() {
	return _size;
}

int Communication::rank() {
	return _rank;
}
