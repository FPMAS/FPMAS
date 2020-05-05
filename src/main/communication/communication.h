#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <string>
#include <mpi.h>
#include <unordered_map>

#include "api/communication/communication.h"

namespace FPMAS {
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
		class MpiCommunicator : public virtual api::communication::MpiCommunicator {
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
				MpiCommunicator& operator=(const MpiCommunicator&);
				MpiCommunicator(std::initializer_list<int>);
				MPI_Group getMpiGroup() const;
				MPI_Comm getMpiComm() const;

				int getRank() const override;
				int getSize() const override;

				void send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) override;
				void Issend(
					const void* data, int count, MPI_Datatype datatype, int destination, int tag, MPI_Request* req) override;

				void send(int destination, int tag) override;
				void Issend(int destination, int tag, MPI_Request* req) override;

				void recv(MPI_Status* status) override;
				void recv(void* buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status) override;

				void probe(int source, int tag, MPI_Status*) override;
				bool Iprobe(int source, int tag, MPI_Status*) override;

				bool test(MPI_Request*) override;

				std::unordered_map<int, std::string> 
					allToAll (
							std::unordered_map<int, std::string> 
							data_pack) override {

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

				~MpiCommunicator();

		};

		template<typename T>
			class TypedMpi : public api::communication::TypedMpi<T> {
				private:
					api::communication::MpiCommunicator& comm;
				public:
					TypedMpi(api::communication::MpiCommunicator& comm) : comm(comm) {}

					std::unordered_map<int, std::vector<T>>
						migrate(std::unordered_map<int, std::vector<T>> exportMap) override;

					void send(const T&, int, int) override;
					void Issend(const T&, int, int, MPI_Request*) override;
					T recv(MPI_Status*) override;
			};

		template<typename T> std::unordered_map<int, std::vector<T>>
			TypedMpi<T>::migrate(std::unordered_map<int, std::vector<T>> exportMap) {
				// Pack
				std::unordered_map<int, std::string> data_pack;
				for(auto item : exportMap) {
					data_pack[item.first] = nlohmann::json(item.second).dump();
				}

				std::unordered_map<int, std::string> data_unpack
					= comm.allToAll(data_pack);

				std::unordered_map<int, std::vector<T>> importMap;
				for(auto item : data_unpack) {
					importMap[item.first] = nlohmann::json::parse(data_unpack[item.first])
						.get<std::vector<T>>();
				}
				return importMap;
			}

		template<typename T>
			void TypedMpi<T>::send(const T& data, int destination, int tag) {
				std::string str = nlohmann::json(data).dump();
				comm.send(str.c_str(), str.size()+1, MPI_CHAR, destination, tag);
			}
		template<typename T>
			void TypedMpi<T>::Issend(const T& data, int destination, int tag, MPI_Request* req) {
				std::string str = nlohmann::json(data).dump();
				comm.Issend(str.c_str(), str.size()+1, MPI_CHAR, destination, tag, req);
			}

		template<typename T>
			T TypedMpi<T>::recv(MPI_Status* status) {
				int count;
				MPI_Get_count(status, MPI_CHAR, &count);
				char* buffer = (char*) std::malloc(count * sizeof(char));
				comm.recv(buffer, count, MPI_CHAR, status->MPI_SOURCE, status->MPI_TAG, MPI_STATUS_IGNORE);

				std::string data (buffer, count);
				std::free(buffer);
				return nlohmann::json::parse(data).get<T>();
			}
	}
}
#endif
