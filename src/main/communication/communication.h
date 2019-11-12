#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <mpi.h>

class Communication {
	private:
		int _size;
		int _rank;

		MPI_Group worldGroup;

	public:
		Communication();

		int size();
		int rank();
		MPI_Group getWorldGroup();

};
#endif
