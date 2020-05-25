#include "communication.h"
#include <nlohmann/json.hpp>
#include <cstdarg>

#include "utils/macros.h"

namespace FPMAS::communication {
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
	MpiCommunicator& FPMAS::communication::MpiCommunicator::operator=(const MpiCommunicator& other) {
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

/*
 *    void MpiCommunicator::send(int destination, int tag) {
 *        MPI_Send(NULL, 0, MPI_CHAR, destination, tag, this->comm);
 *
 *    }
 *
 *    void MpiCommunicator::send(Color token, int destination, int tag) {
 *        MPI_Send(&token, 1, MPI_INT, destination, tag, this->comm);
 *    }
 *
 *    void MpiCommunicator::send(const std::string& str, int destination, int tag) {
 *        MPI_Send(str.c_str(), str.size() +1, MPI_CHAR, destination, tag, this->comm);
 *    }
 *
 *    void MpiCommunicator::sendEnd(int destination) {
 *        MPI_Send(NULL, 0, MPI_INT, destination, Tag::END, this->comm);
 *    }
 *
 *    void MpiCommunicator::Issend(const DistributedId& id, int destination, int tag, MPI_Request* req) {
 *        MpiDistributedId mpi_id;
 *        mpi_id.rank = id.rank();
 *        mpi_id.id = id.id();
 *        MPI_Issend(&mpi_id, 1, MPI_DISTRIBUTED_ID_TYPE, destination, tag, this->comm, req);
 *    }
 *
 *    void MpiCommunicator::Issend(const std::string& str, int destination, int tag, MPI_Request* req) {
 *        MPI_Issend(str.c_str(), str.length() + 1, MPI_CHAR, destination, tag, this->comm, req);
 *    }
 *
 *    void MpiCommunicator::Isend(const std::string& str, int destination, int tag, MPI_Request* req) {
 *        MPI_Isend(str.c_str(), str.length() + 1, MPI_CHAR, destination, tag, this->comm, req);
 *    }
 */
	void MpiCommunicator::send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) {
		MPI_Send(data, count, datatype, destination, tag, this->comm);
	}

	void MpiCommunicator::send(int destination, int tag) {
		MPI_Send(NULL, 0, MPI_CHAR, destination, tag, this->comm);
	}

	void MpiCommunicator::Issend(
			const void* data, int count, MPI_Datatype datatype, int destination, int tag, MPI_Request* req) {
		MPI_Issend(data, count, datatype, destination, tag, this->comm, req);
	}

	void MpiCommunicator::Issend(int destination, int tag, MPI_Request* req) {
		MPI_Issend(NULL, 0, MPI_CHAR, destination, tag, this->comm, req);
	}

	void MpiCommunicator::recv(MPI_Status* status) {
		MPI_Recv(NULL, 0, MPI_INT, status->MPI_SOURCE, status->MPI_TAG, this->comm, status);
	}

	void MpiCommunicator::recv(
		void* buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status) {
		MPI_Recv(buffer, count, datatype, source, tag, this->comm, status);
	}

	void MpiCommunicator::probe(int source, int tag, MPI_Status* status) {
		MPI_Probe(source, tag, this->comm, status);
	}

	bool MpiCommunicator::Iprobe(int source, int tag, MPI_Status* status) {
		int flag;
		MPI_Iprobe(source, tag, this->comm, &flag, status);
		return flag > 0;
	}

	bool MpiCommunicator::test(MPI_Request* req) {
		MPI_Status status;
		int flag;
		MPI_Test(req, &flag, &status);
		return flag > 0;
	}

/*
 *    void MpiCommunicator::recv(MPI_Status* status, Color& data) {
 *        int _data;
 *        MPI_Status _status;
 *        MPI_Recv(&_data, 1, MPI_INT, status->MPI_SOURCE, status->MPI_TAG, this->comm, status);
 *        data = Color(_data);
 *    }
 *
 *    void MpiCommunicator::recv(MPI_Status* status, std::string& str) {
 *        int count;
 *        MPI_Get_count(status, MPI_CHAR, &count);
 *        char data[count];
 *        MPI_Recv(&data, count, MPI_CHAR, status->MPI_SOURCE, status->MPI_TAG, this->comm, status);
 *        str = std::string(data);
 *    }
 *
 *    void MpiCommunicator::recv(MPI_Status* status, DistributedId& id) {
 *        MpiDistributedId mpi_id;
 *        MPI_Recv(&mpi_id, 1, MPI_DISTRIBUTED_ID_TYPE, status->MPI_SOURCE, status->MPI_TAG, this->comm, status);
 *        id = DistributedId(mpi_id);
 *    }
 */
	std::unordered_map<int, std::string> 
		MpiCommunicator::allToAll (
				std::unordered_map<int, std::string> 
				data_pack) {

			// Migrate
			int sendcounts[getSize()];
			int sdispls[getSize()];

			int size_buffer[getSize()];

			int current_sdispls = 0;
			for (int i = 0; i < getSize(); i++) {
				sendcounts[i] = data_pack[i].size();
				sdispls[i] = current_sdispls;
				current_sdispls += sendcounts[i];

				size_buffer[i] = sendcounts[i];
			}
			char send_buffer[current_sdispls];
			for(int i = 0; i < getSize(); i++) {
				std::strncpy(&send_buffer[sdispls[i]], data_pack[i].c_str(), sendcounts[i]);
			}

			// Sends size / displs to each rank, and receive recvs size / displs from
			// each rank.
			MPI_Alltoall(MPI_IN_PLACE, 0, MPI_INT, size_buffer, 1, MPI_INT, getMpiComm());

			int recvcounts[getSize()];
			int rdispls[getSize()];
			int current_rdispls = 0;
			for (int i = 0; i < getSize(); i++) {
				recvcounts[i] = size_buffer[i];
				rdispls[i] = current_rdispls;
				current_rdispls += recvcounts[i];
			}

			char recv_buffer[current_rdispls];

			MPI_Alltoallv(
					send_buffer, sendcounts, sdispls, MPI_CHAR,
					recv_buffer, recvcounts, rdispls, MPI_CHAR,
					getMpiComm()
					);

			std::unordered_map<int, std::string> data_unpack;
			for (int i = 0; i < getSize(); i++) {
				if(recvcounts[i] > 0)
					data_unpack[i]= std::string(&recv_buffer[rdispls[i]], recvcounts[i]);
			}
			return data_unpack;
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
