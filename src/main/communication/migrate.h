#ifndef MIGRATE_H
#define MIGRATE_H

#include "mpi.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "nlohmann/json.hpp"

#include "communication/communication.h"

using nlohmann::json;
using FPMAS::communication::MpiCommunicator;

namespace FPMAS::communication {

	template<typename T, typename Data = std::nullptr_t > std::unordered_map<int, std::vector<T>> migrate(
			std::unordered_map<int, std::vector<T>> export_map,
			MpiCommunicator& comm,
			Data data = nullptr 
			) {
		// Pack
		std::unordered_map<int, std::string> data_pack;
		for(auto item : export_map) {
			data_pack[item.first] = json(item.second).dump();
		}

		int sendcounts[comm.getSize()];
		int sdispls[comm.getSize()];

		int size_buffer[comm.getSize()];

		int current_sdispls = 0;
		for (int i = 0; i < comm.getSize(); i++) {
			sendcounts[i] = data_pack[i].size();
			sdispls[i] = current_sdispls;
			std::cout << i << ": " << data_pack[i] << std::endl;
			std::cout << i << ":scount " << sendcounts[i] << std::endl;
			std::cout << i << ":sdispls " << sdispls[i] << std::endl;
			current_sdispls += sendcounts[i];

			size_buffer[i] = sendcounts[i];
		}
		char send_buffer[current_sdispls];
		for(int i = 0; i < comm.getSize(); i++) {
			std::cout << data_pack[i].c_str() << std::endl;
			std::strncpy(&send_buffer[sdispls[i]], data_pack[i].c_str(), sendcounts[i]);
		}
		for(int i = 0; i < current_sdispls; i++){
			std::cout << send_buffer[i];
		}
		std::cout << std::endl;

		// Sends size / displs to each rank, and receive recvs size / displs from
		// each rank.
		MPI_Alltoall(MPI_IN_PLACE, 0, MPI_INT, size_buffer, 1, MPI_INT, comm.getMpiComm());

		int recvcounts[comm.getSize()];
		int rdispls[comm.getSize()];
		int current_rdispls = 0;
		for (int i = 0; i < comm.getSize(); i++) {
			recvcounts[i] = size_buffer[i];
			std::cout << "[" << comm.getRank() << "]" << i << ": rcount " << recvcounts[i] << std::endl;
			rdispls[i] = current_rdispls;
			current_rdispls += recvcounts[i];
		}

		char recv_buffer[current_rdispls];

		MPI_Alltoallv(
			send_buffer, sendcounts, sdispls, MPI_CHAR,
			recv_buffer, recvcounts, rdispls, MPI_CHAR,
			comm.getMpiComm()
			);

		for(int i = 0; i < current_rdispls; i++) {
			std::cout << recv_buffer[i];
		}
		std::cout << std::endl;

		std::unordered_map<int, std::string> data_unpack;
		for (int i = 0; i < comm.getSize(); i++) {
			if(recvcounts[i] > 0)
				data_unpack[i]= std::string(&recv_buffer[rdispls[i]], recvcounts[i]);
		}

		std::unordered_map<int, std::vector<T>> import_map;
		for(auto item : data_unpack) {
			import_map[item.first] = json::parse(data_unpack[item.first])
				.get<std::vector<T>>();
		}

		return import_map;
	}
}

#endif
