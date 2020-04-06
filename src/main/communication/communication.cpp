#include "communication.h"
#include <nlohmann/json.hpp>
#include <cstdarg>

#include "utils/macros.h"

namespace FPMAS::communication {
	using FPMAS::api::communication::Tag;

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
	 * FPMAS::communication::MpiCommunicator comm {0, 1, 2};
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
	 * FPMAS::communication::MpiCommunicator copy constructor.
	 *
	 * Allows a correct MPI resources management.
	 *
	 * @param from FPMAS::communication::MpiCommunicator to copy from
	 */
	MpiCommunicator::MpiCommunicator(const FPMAS::communication::MpiCommunicator& from) {
		this->comm = from.comm;
		this->comm_references = from.comm_references + 1;

		MPI_Comm_group(this->comm, &this->group);

		MPI_Comm_rank(this->comm, &this->rank);
		MPI_Comm_size(this->comm, &this->size);

	}
	/**
	 * FPMAS::communication::MpiCommunicator copy assignment.
	 *
	 * Allows a correct MPI resources management.
	 *
	 * @param other FPMAS::communication::MpiCommunicator to assign to this
	 */
	MpiCommunicator& FPMAS::communication::MpiCommunicator::operator=(MpiCommunicator other) {
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

	void MpiCommunicator::send(Color token, int destination) {
		MPI_Send(&token, 1, MPI_INT, destination, Tag::TOKEN, this->getMpiComm());
	}

	void MpiCommunicator::sendEnd(int destination) {
		MPI_Send(NULL, 0, MPI_INT, destination, Tag::END, this->getMpiComm());
	}

	void MpiCommunicator::Issend(const DistributedId& id, int destination, int tag, MPI_Request* req) {
		MpiDistributedId mpi_id;
		mpi_id.rank = id.rank();
		mpi_id.id = id.id();
		MPI_Issend(&mpi_id, 1, MPI_DISTRIBUTED_ID_TYPE, destination, tag, this->getMpiComm(), req);
	}

	void MpiCommunicator::Issend(const std::string& str, int destination, int tag, MPI_Request* req) {
		MPI_Issend(str.c_str(), str.length() + 1, MPI_CHAR, destination, tag, this->getMpiComm(), req);
	}

	void MpiCommunicator::Isend(const std::string& str, int destination, int tag, MPI_Request* req) {
		MPI_Isend(str.c_str(), str.length() + 1, MPI_CHAR, destination, tag, this->getMpiComm(), req);
	}

	void MpiCommunicator::probe(int source, int tag, MPI_Status * status) {
		MPI_Probe(source, tag, this->getMpiComm(), status);
	}

	int MpiCommunicator::Iprobe(int source, int tag, MPI_Status* status) {
		int flag;
		MPI_Iprobe(source, tag, this->getMpiComm(), &flag, status);
		return flag;
	}

	void MpiCommunicator::recvEnd(MPI_Status* status) {
		MPI_Recv(NULL, 0, MPI_INT, status->MPI_SOURCE, status->MPI_TAG, this->getMpiComm(), status);
	}

	void MpiCommunicator::recv(MPI_Status * status, Color& data) {
		int _data;
		MPI_Recv(&_data, 1, MPI_INT, status->MPI_SOURCE, status->MPI_TAG, this->getMpiComm(), status);
		data = Color(_data);
	}

	void MpiCommunicator::recv(MPI_Status * status, std::string& str) {
		int count;
		MPI_Get_count(status, MPI_CHAR, &count);
		char data[count];
		MPI_Recv(&data, count, MPI_CHAR, status->MPI_SOURCE, status->MPI_TAG, this->getMpiComm(), status);
		str = std::string(data);
	}

	void MpiCommunicator::recv(MPI_Status * status, DistributedId& id) {
		MpiDistributedId mpi_id;
		MPI_Recv(&mpi_id, 1, MPI_DISTRIBUTED_ID_TYPE, status->MPI_SOURCE, status->MPI_TAG, this->getMpiComm(), status);
		id = DistributedId(mpi_id);
	}

	/**
	 * FPMAS::communication::MpiCommunicator destructor.
	 *
	 * Properly releases acquired MPI resources.
	 */
	MpiCommunicator::~MpiCommunicator() {
		MPI_Group_free(&this->group);
		if(this->comm_references == 1)
			MPI_Comm_free(&this->comm);
	}
}
