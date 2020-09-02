#include "communication.h"
#include <nlohmann/json.hpp>
#include <cstdarg>

#include "fpmas/utils/macros.h"

namespace fpmas { namespace communication {
	MpiCommunicator::MpiCommunicator() {
		MPI_Group worldGroup;
		MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
		MPI_Group_union(worldGroup, MPI_GROUP_EMPTY, &this->group);
		MPI_Comm_create(MPI_COMM_WORLD, this->group, &this->comm);

		MPI_Comm_rank(this->comm, &this->rank);
		MPI_Comm_size(this->comm, &this->size);

		MPI_Group_free(&worldGroup);
	}

	void MpiCommunicator::convertStatus(MPI_Status& mpi_status, Status& status, MPI_Datatype datatype) {
		MPI_Get_count(&mpi_status, datatype, &status.item_count);
		int size;
		MPI_Type_size(datatype, &size);
		status.size = size * status.item_count;
		status.source = mpi_status.MPI_SOURCE;
		status.tag = mpi_status.MPI_TAG;
	}

	MPI_Comm MpiCommunicator::getMpiComm() const {
		return this->comm;
	}

	MPI_Group MpiCommunicator::getMpiGroup() const {
		return this->group;
	}

	int MpiCommunicator::getRank() const {
		return this->rank;
	}

	int MpiCommunicator::getSize() const {
		return this->size;
	}

	void MpiCommunicator::send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) {
		MPI_Send(data, count, datatype, destination, tag, this->comm);
	}

	void MpiCommunicator::send(int destination, int tag) {
		MPI_Send(NULL, 0, MPI_CHAR, destination, tag, this->comm);
	}

	void MpiCommunicator::Issend(
			const void* data, int count, MPI_Datatype datatype, int destination, int tag, Request& req) {
		int type_size;
		MPI_Type_size(datatype, &type_size);
		req.__data = new DataPack(count, type_size);
		std::memcpy(req.__data->buffer, data, req.__data->size);

		MPI_Issend(req.__data->buffer, count, datatype, destination, tag, this->comm, &req.__mpi_request);
	}

	void MpiCommunicator::Issend(int destination, int tag, Request& req) {
		MPI_Issend(NULL, 0, MPI_CHAR, destination, tag, this->comm, &req.__mpi_request);
	}

	void MpiCommunicator::recv(int source, int tag, Status& status) {
		MPI_Status __status;
		MPI_Recv(NULL, 0, IGNORE_TYPE, source, tag, this->comm, &__status);
		status.item_count = 0;
		status.size = 0;
		status.source = __status.MPI_SOURCE;
		status.tag = __status.MPI_TAG;
	}

	void MpiCommunicator::recv(
			void* buffer, int count, MPI_Datatype datatype, int source, int tag, Status& status) {
		MPI_Status __status {};
		MPI_Recv(buffer, count, datatype, source, tag, this->comm, &__status);
		convertStatus(__status, status, datatype);
	}

	void MpiCommunicator::probe(MPI_Datatype type, int source, int tag, Status& status) {
		MPI_Status __status {};
		MPI_Probe(source, tag, this->comm, &__status);
		convertStatus(__status, status, type);
	}

	bool MpiCommunicator::Iprobe(MPI_Datatype type, int source, int tag, Status& status) {
		int flag;
		MPI_Status __status {};
		MPI_Iprobe(source, tag, this->comm, &flag, &__status);
		convertStatus(__status, status, type);
		return flag > 0;
	}

	bool MpiCommunicator::test(Request& req) {
		MPI_Status status;
		int flag;
		MPI_Test(&req.__mpi_request, &flag, &status);
		if(flag > 0) {
			req.free();
		}
		return flag > 0;
	}

	void MpiCommunicator::wait(Request& req) {
		MPI_Status status;
		MPI_Wait(&req.__mpi_request, &status);
		req.free();
	}

	std::unordered_map<int, DataPack> 
		MpiCommunicator::allToAll (
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
				std::memcpy(&((char*) send_buffer)[sdispls[i]], data_pack[i].buffer, data_pack[i].size);
			}

			int* recvcounts = (int*) std::malloc(getSize()*sizeof(int));
			int* rdispls = (int*) std::malloc(getSize()*sizeof(int));
			int current_rdispls = 0;
			for (int i = 0; i < getSize(); i++) {
				recvcounts[i] = size_buffer[i];
				rdispls[i] = current_rdispls;
				current_rdispls += recvcounts[i];
			}

			void* recv_buffer = std::malloc(current_rdispls * type_size);

			MPI_Alltoallv(
					send_buffer, sendcounts, sdispls, datatype,
					recv_buffer, recvcounts, rdispls, datatype,
					getMpiComm()
					);

			std::unordered_map<int, DataPack> imported_data_pack;
			for (int i = 0; i < getSize(); i++) {
				if(recvcounts[i] > 0) {
					imported_data_pack[i] = DataPack(recvcounts[i], type_size);
					DataPack& dataPack = imported_data_pack[i];

					std::memcpy(dataPack.buffer, &((char*) recv_buffer)[rdispls[i]], dataPack.size);
				}
			}

			std::free(sendcounts);
			std::free(sdispls);
			std::free(size_buffer);
			std::free(recvcounts);
			std::free(rdispls);
			std::free(send_buffer);
			std::free(recv_buffer);
			return imported_data_pack;
		}

	std::vector<DataPack> MpiCommunicator::gather(DataPack data, MPI_Datatype type, int root) {

		int type_size;
		MPI_Type_size(type, &type_size);


		void* send_buffer = std::malloc(data.count * type_size);
		std::memcpy(&((char*) send_buffer)[0], data.buffer, data.size);


		int* size_buffer;
		//int recv_size_count = 0;
		if(getRank() == root) {
			//recv_size_count = getSize();
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

		MPI_Gatherv(send_buffer, data.count, type, recv_buffer, recvcounts, rdispls, type, root, comm);

		std::vector<DataPack> imported_data_pack;
		if(getRank() == root) {
			for (int i = 0; i < getSize(); i++) {
				imported_data_pack.emplace_back(recvcounts[i], type_size);
				DataPack& data_pack = imported_data_pack[i];

				std::memcpy(data_pack.buffer, &((char*) recv_buffer)[rdispls[i]], data_pack.size);
			}
		}
		std::free(send_buffer);
		std::free(size_buffer);
		std::free(recvcounts);
		std::free(rdispls);
		std::free(recv_buffer);
		return imported_data_pack;
	}

	MpiCommunicator::~MpiCommunicator() {
		MPI_Group_free(&this->group);
		MPI_Comm_free(&this->comm);
	}
}}
