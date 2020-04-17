#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <string>
#include <mpi.h>
#include <unordered_map>

#include "api/communication/communication.h"

namespace FPMAS {
	using api::communication::Color;
	/**
	 * The FPMAS::communication namespace contains low level MPI features
	 * required by communication processes.
	 */
	namespace communication {
		/**
		 * A convenient wrapper to build MPI groups and communicators from the
		 * MPI_COMM_WORLD communicator, i.e. groups and communicators
		 * containing all the processes available.
		 */
		class MpiCommunicator : public virtual api::communication::MpiCommunicator<MpiCommunicator> {
			private:
				int size;
				int rank;

				MPI_Group group;
				MPI_Comm comm;

				/*
				 * About the comm_references usage :
				 * The MPI reference only defines the MPI_Comm_dup function to copy
				 * communicators, what requires MPI calls on all other procs to be performed. In
				 * consequence, MPI_Comm_dup is :
				 * 	1- Oversized for our need
				 * 	2- Impossible to use in practice, because all procs would have to sync each
				 * 	time a copy constructor is called...
				 *
				 * 	Moreover, it is safe in our context for all the copied / re-assigned
				 * 	MpiCommunicator from this instance to use the same MPI_Comm instance.
				 * 	However, calling MPI_Comm_free multiple time on the same MPI_Comm instance
				 * 	results in erroneous MPI memory usage (this has been verified with
				 * 	valgrind). In consequence, we use the comm_references variable to smartly
				 * 	manage the original MPI_Comm instance, in order to free it only when all
				 * 	the MpiCommunicator instances using it have been destructed.
				 */
				int comm_references = 1;

			public:
				MpiCommunicator();
				MpiCommunicator(const MpiCommunicator&);
				MpiCommunicator& operator=(MpiCommunicator);
				MpiCommunicator(std::initializer_list<int>);
				MPI_Group getMpiGroup() const;
				MPI_Comm getMpiComm() const;

				int getRank() const override;
				int getSize() const override;

				void send(Color token, int destination) override;
				void sendEnd(int destination) override;
				void Issend(const DistributedId&, int destination, int tag, MPI_Request*) override;
				void Issend(const std::string&, int destination, int tag, MPI_Request*) override;
				void Isend(const std::string&, int destination, int tag, MPI_Request*) override;

				void recvEnd(MPI_Status*) override;
				void recv(MPI_Status*, Color&) override;
				void recv(MPI_Status*, std::string&) override;
				void recv(MPI_Status*, DistributedId&) override;

				void probe(int source, int tag, MPI_Status*) override;
				int Iprobe(int source, int tag, MPI_Status*) override;

				template<typename T> std::unordered_map<int, std::vector<T>>
					migrate(std::unordered_map<int, std::vector<T>>);

				~MpiCommunicator();

		};

		template<typename T> std::unordered_map<int, std::vector<T>>
			MpiCommunicator::migrate(std::unordered_map<int, std::vector<T>> exportMap) {
				// Pack
				std::unordered_map<int, std::string> data_pack;
				for(auto item : exportMap) {
					data_pack[item.first] = json(item.second).dump();
				}

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

				std::unordered_map<int, std::vector<T>> importMap;
				for(auto item : data_unpack) {
					importMap[item.first] = nlohmann::json::parse(data_unpack[item.first])
						.get<std::vector<T>>();
				}

				return importMap;
			}
	}
}
#endif
