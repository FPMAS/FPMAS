#include "communication.h"
#include "mpi.h"

Communication::Communication() {
	this->_rank=MPI::COMM_WORLD.Get_rank();
	this->_size=MPI::COMM_WORLD.Get_size();
}

uint16_t Communication::size() {
	return _size;
}

uint16_t Communication::rank() {
	return _rank;
}
