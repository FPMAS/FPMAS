#ifndef MIGRATE_H
#define MIGRATE_H

#include "mpi.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "nlohmann/json.hpp"

#include "communication/communication.h"

using nlohmann::json;

namespace FPMAS::communication {

	template<
		typename T
		> std::unordered_map<int, std::vector<T>> migrate(
			std::unordered_map<int, std::vector<T>> exportMap,
			MpiCommunicator& comm
			//Data& data
			) {

		// Pack
		std::unordered_map<int, std::string> data_pack;
		for(auto item : exportMap) {
			data_pack[item.first] = json(item.second).dump();
		}

		// Migrate
		int sendcounts[comm.getSize()];
		int sdispls[comm.getSize()];

		int size_buffer[comm.getSize()];

		int current_sdispls = 0;
		for (int i = 0; i < comm.getSize(); i++) {
			sendcounts[i] = data_pack[i].size();
			sdispls[i] = current_sdispls;
			current_sdispls += sendcounts[i];

			size_buffer[i] = sendcounts[i];
		}
		char send_buffer[current_sdispls];
		for(int i = 0; i < comm.getSize(); i++) {
			std::strncpy(&send_buffer[sdispls[i]], data_pack[i].c_str(), sendcounts[i]);
		}

		// Sends size / displs to each rank, and receive recvs size / displs from
		// each rank.
		MPI_Alltoall(MPI_IN_PLACE, 0, MPI_INT, size_buffer, 1, MPI_INT, comm.getMpiComm());

		int recvcounts[comm.getSize()];
		int rdispls[comm.getSize()];
		int current_rdispls = 0;
		for (int i = 0; i < comm.getSize(); i++) {
			recvcounts[i] = size_buffer[i];
			rdispls[i] = current_rdispls;
			current_rdispls += recvcounts[i];
		}

		char recv_buffer[current_rdispls];

		MPI_Alltoallv(
			send_buffer, sendcounts, sdispls, MPI_CHAR,
			recv_buffer, recvcounts, rdispls, MPI_CHAR,
			comm.getMpiComm()
			);

		std::unordered_map<int, std::string> data_unpack;
		for (int i = 0; i < comm.getSize(); i++) {
			if(recvcounts[i] > 0)
				data_unpack[i]= std::string(&recv_buffer[rdispls[i]], recvcounts[i]);
		}

		std::unordered_map<int, std::vector<T>> importMap;
		for(auto item : data_unpack) {
			importMap[item.first] = json::parse(data_unpack[item.first])
				.get<std::vector<T>>();
		}

		return importMap;
	}
}

#endif
