#include "communication.h"
#include <nlohmann/json.hpp>
#include <cstdarg>

#include "fpmas/utils/macros.h"

namespace fpmas { namespace communication {

	MpiCommWorld WORLD {};

	void MpiCommunicatorBase::convertStatus(MPI_Status& mpi_status, Status& status, MPI_Datatype datatype) {
		MPI_Get_count(&mpi_status, datatype, &status.item_count);
		int size;
		MPI_Type_size(datatype, &size);
		status.size = size * status.item_count;
		status.source = mpi_status.MPI_SOURCE;
		status.tag = mpi_status.MPI_TAG;
	}

	MPI_Comm MpiCommunicatorBase::getMpiComm() const {
		return this->comm;
	}

	MPI_Group MpiCommunicatorBase::getMpiGroup() const {
		return this->group;
	}

	int MpiCommunicatorBase::getRank() const {
		return this->rank;
	}

	int MpiCommunicatorBase::getSize() const {
		return this->size;
	}

	void MpiCommunicatorBase::send(
			const void* data, int count, MPI_Datatype datatype,
			int destination, int tag) {
		MPI_Send(data, count, datatype, destination, tag, this->comm);
	}

	void MpiCommunicatorBase::send(
			const DataPack& data, MPI_Datatype datatype,
			int destination, int tag) {
		send(data.buffer, data.count, datatype, destination, tag);
	}

	void MpiCommunicatorBase::send(int destination, int tag) {
		MPI_Send(NULL, 0, MPI_CHAR, destination, tag, this->comm);
	}

	void MpiCommunicatorBase::Issend(
			const void* data, int count, MPI_Datatype datatype,
			int destination, int tag, Request& req) {
		int type_size;
		MPI_Type_size(datatype, &type_size);
		req.__data = new DataPack(count, type_size);
		std::memcpy(req.__data->buffer, data, req.__data->size);

		MPI_Issend(
				req.__data->buffer, count, datatype,
				destination, tag, this->comm, &req.__mpi_request);
	}
	void MpiCommunicatorBase::Issend(
			const DataPack& data, MPI_Datatype datatype,
			int destination, int tag, Request& req) {
		Issend(data.buffer, data.count, datatype, destination, tag, req);
	}

	void MpiCommunicatorBase::Issend(int destination, int tag, Request& req) {
		MPI_Issend(NULL, 0, MPI_CHAR, destination, tag, this->comm, &req.__mpi_request);
	}

	void MpiCommunicatorBase::recv(int source, int tag, Status& status) {
		MPI_Status __status;
		MPI_Recv(NULL, 0, IGNORE_TYPE, source, tag, this->comm, &__status);
		status.item_count = 0;
		status.size = 0;
		status.source = __status.MPI_SOURCE;
		status.tag = __status.MPI_TAG;
	}

	void MpiCommunicatorBase::recv(
			void* buffer, int count, MPI_Datatype datatype, int source, int tag, Status& status) {
		MPI_Status __status {};
		MPI_Recv(buffer, count, datatype, source, tag, this->comm, &__status);
		convertStatus(__status, status, datatype);
	}
	void MpiCommunicatorBase::recv(
			DataPack& data, MPI_Datatype datatype, int source, int tag, Status& status) {
		recv(data.buffer, data.count, datatype, source, tag, status);
	}

	void MpiCommunicatorBase::probe(MPI_Datatype type, int source, int tag, Status& status) {
		MPI_Status __status {};
		MPI_Probe(source, tag, this->comm, &__status);
		convertStatus(__status, status, type);
	}

	bool MpiCommunicatorBase::Iprobe(MPI_Datatype type, int source, int tag, Status& status) {
		int flag;
		MPI_Status __status {};
		MPI_Iprobe(source, tag, this->comm, &flag, &__status);
		if(flag > 0)
			convertStatus(__status, status, type);
		return flag > 0;
	}

	bool MpiCommunicatorBase::test(Request& req) {
		MPI_Status status;
		int flag;
		MPI_Test(&req.__mpi_request, &flag, &status);
		if(flag > 0) {
			req.free();
		}
		return flag > 0;
	}

	void MpiCommunicatorBase::wait(Request& req) {
		MPI_Status status;
		MPI_Wait(&req.__mpi_request, &status);
		req.free();
	}

	std::unordered_map<int, DataPack> 
		MpiCommunicatorBase::allToAll (
				std::unordered_map<int, DataPack> 
				data_pack, MPI_Datatype datatype) {
			// Migrate
			int* sendcounts = (int*) std::malloc(getSize()*sizeof(int));
			int* sdispls = (int*) std::malloc(getSize()*sizeof(int));

			int* size_buffer = (int*) std::malloc(getSize()*sizeof(int));

			int current_sdispls = 0;
			for (int i = 0; i < getSize(); i++) {
				sendcounts[i] = data_pack[i].count;
				sdispls[i] = current_sdispls;
				current_sdispls += sendcounts[i];

				size_buffer[i] = sendcounts[i];
			}

			// Sends size / displs to each rank, and receive recvs size / displs from
			// each rank.
			MPI_Alltoall(MPI_IN_PLACE, 0, MPI_INT, size_buffer, 1, MPI_INT, getMpiComm());

			int type_size;
			MPI_Type_size(datatype, &type_size);

			void* send_buffer = std::malloc(current_sdispls * type_size);
			for(int i = 0; i < getSize(); i++) {
				DataPack pack = std::move(data_pack[i]);
				std::memcpy(&((char*) send_buffer)[sdispls[i]], pack.buffer, pack.size);
				// The temporary data pack is freed
			}

			int* recvcounts = (int*) std::malloc(getSize()*sizeof(int));
			int* rdispls = (int*) std::malloc(getSize()*sizeof(int));
			int current_rdispls = 0;
			for (int i = 0; i < getSize(); i++) {
				recvcounts[i] = size_buffer[i];
				rdispls[i] = current_rdispls;
				current_rdispls += recvcounts[i];
			}
			// Frees useless buffer
			std::free(size_buffer);

			// Allocates buffer where MPI data are received
			void* recv_buffer = std::malloc(current_rdispls * type_size);

			MPI_Alltoallv(
					send_buffer, sendcounts, sdispls, datatype,
					recv_buffer, recvcounts, rdispls, datatype,
					getMpiComm()
					);
			// Frees useless send data buffers
			std::free(sendcounts);
			std::free(sdispls);
			std::free(send_buffer);

			// Convert received MPI data to DataPacks
			std::unordered_map<int, DataPack> imported_data_pack;
			for (int i = 0; i < getSize(); i++) {
				if(recvcounts[i] > 0) {
					DataPack pack(recvcounts[i], type_size);

					std::memcpy(pack.buffer, &((char*) recv_buffer)[rdispls[i]], pack.size);
					imported_data_pack.emplace(i, std::move(pack));
				}
			}

			// Frees receive data buffers
			std::free(recvcounts);
			std::free(rdispls);
			std::free(recv_buffer);

			// Should perform "copy elision" to prevent useless buffer copies
			return imported_data_pack;
		}

	std::vector<DataPack> MpiCommunicatorBase::gather(DataPack data, MPI_Datatype type, int root) {

		int type_size;
		MPI_Type_size(type, &type_size);


		int* size_buffer;
		if(getRank() == root) {
			size_buffer = (int*) std::malloc(getSize() * sizeof(int));
		} else {
			size_buffer = (int*) std::malloc(0);
		}

		MPI_Gather(&data.count, 1, MPI_INT, size_buffer, 1, MPI_INT, root, comm);

		int* recvcounts;
		int* rdispls;
		int current_rdispls = 0;
		if(getRank() == root) {
			recvcounts = (int*) std::malloc(getSize()*sizeof(int));
			rdispls = (int*) std::malloc(getSize()*sizeof(int));
			for (int i = 0; i < getSize(); i++) {
				recvcounts[i] = size_buffer[i];
				rdispls[i] = current_rdispls;
				current_rdispls += recvcounts[i];
			}
		} else {
			recvcounts = (int*) std::malloc(0);
			rdispls = (int*) std::malloc(0);
		}
		void* recv_buffer = std::malloc(current_rdispls * type_size);

		MPI_Gatherv(data.buffer, data.count, type, recv_buffer, recvcounts, rdispls, type, root, comm);

		std::vector<DataPack> imported_data_pack;
		if(getRank() == root) {
			for (int i = 0; i < getSize(); i++) {
				imported_data_pack.emplace_back(recvcounts[i], type_size);
				DataPack& data_pack = imported_data_pack[i];

				std::memcpy(data_pack.buffer, &((char*) recv_buffer)[rdispls[i]], data_pack.size);
			}
		}
		std::free(size_buffer);
		std::free(recvcounts);
		std::free(rdispls);
		std::free(recv_buffer);
		return imported_data_pack;
	}

	std::vector<DataPack> MpiCommunicatorBase::allGather(DataPack data, MPI_Datatype type) {
		int type_size;
		MPI_Type_size(type, &type_size);


		int* count_buffer = (int*) std::malloc(getSize() * sizeof(int));

		int local_count = data.count;
		MPI_Allgather(&local_count, 1, MPI_INT, count_buffer, 1, MPI_INT, comm);

		int* rdispls = (int*) std::malloc(getSize()*sizeof(int));
		int current_rdispls = 0;

		for (int i = 0; i < getSize(); i++) {
			rdispls[i] = current_rdispls;
			current_rdispls += count_buffer[i]*type_size;
		}
		
		char* recv_buffer = (char*) std::malloc(current_rdispls);

		MPI_Allgatherv(data.buffer, local_count, type, recv_buffer, count_buffer, rdispls, type, comm);

		std::vector<DataPack> imported_data_pack(getSize());
		for (int i = 0; i < getSize(); i++) {
			imported_data_pack[i] = DataPack(count_buffer[i], type_size);
			DataPack& data_pack = imported_data_pack[i];

			std::memcpy(data_pack.buffer, recv_buffer+rdispls[i], data_pack.size);
		}

		std::free(count_buffer);
		std::free(rdispls);
		std::free(recv_buffer);
		return imported_data_pack;
	}


	DataPack MpiCommunicatorBase::bcast(DataPack data, MPI_Datatype datatype, int root) {
		// Procs other that root don't know how many items will be received, so
		// we broadcast items count from root first
		int count = data.count;
		MPI_Bcast(&count, 1, MPI_INT, root, this->comm);

		// Allocates the in/out buffer used by MPI_Bcast
		int type_size;
		MPI_Type_size(datatype, &type_size);
		if(this->getRank() != root)
			// If not root, reallocate the temporary data buffer so that it can
			// receive data from root
			data = DataPack(count, type_size);
		MPI_Bcast(data.buffer, count, datatype, root, this->comm);
		return data;
	}

	void MpiCommunicatorBase::barrier() {
		MPI_Barrier(this->comm);
	}

	MpiCommunicator::MpiCommunicator() {
		MPI_Group worldGroup;
		MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
		MPI_Group_union(worldGroup, MPI_GROUP_EMPTY, &this->group);
		MPI_Comm_create(MPI_COMM_WORLD, this->group, &this->comm);

		MPI_Comm_rank(this->comm, &this->rank);
		MPI_Comm_size(this->comm, &this->size);

		MPI_Group_free(&worldGroup);
	}

	MpiCommunicator::~MpiCommunicator() {
		MPI_Group_free(&this->group);
		MPI_Comm_free(&this->comm);
	}
}}
