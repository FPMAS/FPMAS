#include "communication.h"
#include <cstdarg>

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
