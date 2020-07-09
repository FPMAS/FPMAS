#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <string>
#include <mpi.h>
#include <unordered_map>

#include "fpmas/api/communication/communication.h"

namespace fpmas {
	/**
	 * The fpmas::communication namespace contains implementations of APIs
	 * defined in fpmas::api::communication.
	 */
	namespace communication {
		using api::communication::DataPack;

		/**
		 * fpmas::api::communication::MpiCommunicator implementation
		 */
		class MpiCommunicator : public virtual api::communication::MpiCommunicator {
			private:
				int size;
				int rank;

				MPI_Group group;
				MPI_Comm comm;

			public:
				MpiCommunicator();
				MpiCommunicator(const MpiCommunicator&) = delete;
				MpiCommunicator& operator=(const MpiCommunicator&) = delete;

				MPI_Group getMpiGroup() const;
				MPI_Comm getMpiComm() const;

				int getRank() const override;
				int getSize() const override;

				void send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) override;
				void Issend(
					const void* data, int count, MPI_Datatype datatype, int destination, int tag, MPI_Request* req) override;

				void send(int destination, int tag) override;
				void Issend(int destination, int tag, MPI_Request* req) override;

				void recv(int source, int tag, MPI_Status* status) override;
				void recv(void* buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status) override;

				void probe(int source, int tag, MPI_Status*) override;
				bool Iprobe(int source, int tag, MPI_Status*) override;

				bool test(MPI_Request*) override;

				std::unordered_map<int, DataPack> 
					allToAll(std::unordered_map<int, DataPack> data_pack, MPI_Datatype datatype) override;
				std::vector<DataPack>
					gather(DataPack, MPI_Datatype, int root) override;

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
					std::vector<T> gather(const T&, int root) override;

					void send(const T&, int, int) override;
					void Issend(const T&, int, int, MPI_Request*) override;
					T recv(int source, int tag, MPI_Status* status = MPI_STATUS_IGNORE) override;
			};

		template<typename T> std::unordered_map<int, std::vector<T>>
			TypedMpi<T>::migrate(std::unordered_map<int, std::vector<T>> exportMap) {
				// Pack
				std::unordered_map<int, DataPack> export_data_pack;
				for(auto item : exportMap) {
					std::string str = nlohmann::json(item.second).dump();
					DataPack data_pack (str.size(), sizeof(char));
					std::memcpy(data_pack.buffer, str.data(), str.size() * sizeof(char));

					export_data_pack[item.first] = data_pack;
				}

				std::unordered_map<int, DataPack> import_data_pack
					= comm.allToAll(export_data_pack, MPI_CHAR);

				std::unordered_map<int, std::vector<T>> importMap;
				for(auto item : import_data_pack) {
					DataPack& pack = import_data_pack[item.first];
					std::string import = std::string((char*) pack.buffer, pack.count);
					importMap[item.first] = nlohmann::json::parse(
							import
							)
						.get<std::vector<T>>();
				}
				return importMap;
			}
		template<typename T> std::vector<T>
			TypedMpi<T>::gather(const T& data, int root) {
				// Pack
				std::string str = nlohmann::json(data).dump();
				DataPack data_pack (str.size(), sizeof(char));
				std::memcpy(data_pack.buffer, str.data(), str.size() * sizeof(char));

				std::vector<DataPack> import_data_pack = comm.gather(data_pack, MPI_CHAR, root);

				std::vector<T> import_data;
				for(auto item : import_data_pack) {
					std::string import = std::string((char*) item.buffer, item.count);
					import_data.push_back(nlohmann::json::parse(import).get<T>());
				}
				return import_data;
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
			T TypedMpi<T>::recv(int source, int tag, MPI_Status* status) {
				MPI_Status message_to_receive_status;
				comm.probe(source, tag, &message_to_receive_status);
				int count;
				MPI_Get_count(&message_to_receive_status, MPI_CHAR, &count);
				char* buffer = (char*) std::malloc(count * sizeof(char));
				comm.recv(buffer, count, MPI_CHAR, source, tag, status);

				std::string data (buffer, count);
				std::free(buffer);
				return nlohmann::json::parse(data).get<T>();
			}
	}
}
#endif
