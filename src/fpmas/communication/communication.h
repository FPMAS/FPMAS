#ifndef FPMAS_COMMUNICATION_H
#define FPMAS_COMMUNICATION_H

/** \file src/fpmas/communication/communication.h
 * Communication implementation.
 */

#include <cstdint>
#include <string>
#include <unordered_map>

#include "fpmas/utils/log.h"
#include "fpmas/api/communication/communication.h"

namespace fpmas { namespace communication {
	using api::communication::DataPack;
	using api::communication::Request;
	using api::communication::Status;

	/**
	 * fpmas::api::communication::MpiCommunicator implementation, based on
	 * the system MPI library (i.e. `#include <mpi.h>`).
	 */
	class MpiCommunicator : public virtual api::communication::MpiCommunicator {
		private:
			int size;
			int rank;

			MPI_Group group;
			MPI_Comm comm;

			static void convertStatus(MPI_Status&, Status&, MPI_Datatype datatype);

		public:
			/**
			 * Default MpiCommunicator constructor.
			 *
			 * Builds an MPI_Group and the associated MPI_Comm as a copy of the
			 * MPI_COMM_WORLD communicator.
			 */
			MpiCommunicator();

			MpiCommunicator(const MpiCommunicator&) = delete;
			MpiCommunicator& operator=(const MpiCommunicator&) = delete;

			/**
			 * Returns the built MPI communicator.
			 *
			 * @return associated MPI communicator
			 */
			MPI_Comm getMpiComm() const;

			/**
			 * Returns the built MPI group.
			 *
			 * @return associated MPI group
			 */
			MPI_Group getMpiGroup() const;

			/**
			 * Returns the MPI rank of this communicator.
			 *
			 * @return MPI communicator rank
			 */
			int getRank() const override;

			/**
			 * Returns the size of this communicator (i.e. the current
			 * processes count).
			 *
			 * @return MPI communicator size
			 */
			int getSize() const override;

			/**
			 * Perfoms an MPI_Send operation.
			 *
			 * @param data input buffer
			 * @param count items count in the buffer
			 * @param datatype MPI type of the data to send
			 * @param destination rank of the destination proc
			 * @param tag message tag
			 */
			void send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) override;
			/**
			 * Performs an MPI_Send operation without data.
			 *
			 * @param destination rank of the destination proc
			 * @param tag message tag
			 */
			void send(int destination, int tag) override;

			/**
			 * Performs an MPI_Issend operation.
			 *
			 * @param data input buffer
			 * @param count items count in the buffer
			 * @param datatype MPI type of the data to send
			 * @param destination rank of the destination proc
			 * @param tag message tag
			 * @param req output MPI request
			 */
			void Issend(
					const void* data, int count, MPI_Datatype datatype, int destination, int tag, Request& req) override;

			/**
			 * Performs an MPI_Issend operation without data.
			 *
			 * @param destination rank of the destination proc
			 * @param tag message tag
			 * @param req output MPI request
			 */
			void Issend(int destination, int tag, Request& req) override;

			/**
			 * Performs an MPI_Recv operation without data.
			 *
			 * @param source rank of the process to receive from
			 * @param tag message tag
			 * @param status output MPI status
			 */
			void recv(int source, int tag, Status& status = Status::IGNORE) override;

			/**
			 * Performs an MPI_Recv operation.
			 *
			 * @param buffer output buffer
			 * @param count items count to receive
			 * @param datatype MPI type of the data to receive
			 * @param source rank of the proc to receive from
			 * @param tag message tag
			 * @param status output MPI status
			 */
			void recv(void* buffer, int count, MPI_Datatype datatype, int source, int tag, Status& status = Status::IGNORE) override;

			/**
			 * Performs an MPI_Probe operation.
			 *
			 * Upon return, the output `status` contains information about the
			 * message to receive.
			 *
			 * @param type expected message type
			 * @param source source rank
			 * @param tag recv tag
			 * @param status MPI status
			 */
			void probe(MPI_Datatype type, int source, int tag, Status&) override;

			/**
			 * Performs an MPI_Iprobe operation.
			 *
			 * Upon return, the output `status` contains information about the
			 * message to receive.
			 *
			 * @param type expected message type
			 * @param source source rank
			 * @param tag recv tag
			 * @param status MPI status
			 * @return true iff a message is available
			 */
			bool Iprobe(MPI_Datatype type, int source, int tag, Status&) override;

			/**
			 * Performs an MPI_Test operation.
			 *
			 * @param req MPI request to test
			 * @returns true iff the request is complete
			 */
			bool test(Request& req) override;

			void wait(Request& req) override;
			/**
			 * Performs an MPI_Alltoall operation.
			 *
			 * Exchanged data are stored in DataPack instances.
			 *
			 * @param export_map data to export to each proc
			 * @param datatype MPI datatype of the data to send / receive
			 * @return data received from each proc
			 */
			std::unordered_map<int, DataPack> 
				allToAll(std::unordered_map<int, DataPack> export_map, MPI_Datatype datatype) override;

			/**
			 * Performs an MPI_Gather operation.
			 *
			 * Exchanged data are stored in DataPack instances.
			 *
			 * @param data data to send to root
			 * @param datatype MPI datatype
			 * @param root rank of the root process
			 * @return if `rank == root`, a vector containing gathered data,
			 * else an empty vector.
			 */
			std::vector<DataPack>
				gather(DataPack data, MPI_Datatype datatype, int root) override;

			DataPack bcast(DataPack data, MPI_Datatype datatype, int root) override;
			
			/**
			 * Performs an MPI_Barrier operation.
			 */
			void barrier() override;

			/**
			 * MpiCommunicator destructor.
			 *
			 * Frees allocated MPI resources.
			 */
			~MpiCommunicator();

	};

	/**
	 * An [nlohmann::json](https://github.com/nlohmann/json) based fpmas::api::communication::TypedMpi
	 * implementation.
	 *
	 * Each `T` instance is serialized as a JSON string using the
	 * nlohmann::json library, and sent as MPI_CHAR using the provided
	 * api::communication::MpiCommunicator.
	 *
	 * This means that ANY TYPE that can be serialized / unserialized as
	 * JSON thanks to the nlohmann::json library can be easily sent across
	 * processors through MPI using this class, preventing users from
	 * struggling with low-level MPI issues and custom MPI_Datatype
	 * definitions.
	 *
	 * Moreover, defining rules to serialize **any custom type** with the
	 * nlohmann::json library is intuitive and straightforward (see
	 * https://github.com/nlohmann/json#arbitrary-types-conversions).
	 *
	 * However, notice that systematically serializing data as JSON might
	 * have an impact on performances, and is not even always necessary (to
	 * send primary types supported by MPI, such as `float` or `int` for
	 * example).
	 *
	 * The interest of using this templated code design is that for such
	 * types, TypedMpi can be (and will be) specialized to circumvent the
	 * default JSON serialization process and use low-level MPI calls
	 * instead, potentially using custom MPI_Datatypes.
	 */
	template<typename T>
		class TypedMpi : public api::communication::TypedMpi<T> {
			private:
				api::communication::MpiCommunicator& comm;
			public:
				/**
				 * TypedMpi constructor.
				 *
				 * The specified MPI communicator will be used to perform
				 * actual message sending between processors.
				 *
				 * To sum up, the role of this class is to serialize /
				 * unserialize input / output data, and transmit it to /
				 * from the underlying api::communication::MpiCommunicator
				 * instance.
				 *
				 * @param comm reference to an MPI communicator instance
				 */
				TypedMpi(api::communication::MpiCommunicator& comm) : comm(comm) {}

				std::unordered_map<int, std::vector<T>>
					migrate(std::unordered_map<int, std::vector<T>> exportMap) override;
				std::vector<T> gather(const T&, int root) override;
				T bcast(const T&, int root) override;

				void send(const T&, int, int) override;
				void Issend(const T&, int, int, Request&) override;

				void probe(int source, int tag, Status& status) override;
				bool Iprobe(int source, int tag, Status& status) override;

				T recv(int source, int tag, Status& status = Status::IGNORE) override;
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
			FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Gather JSON on root %i : %s", root, str.c_str());
			DataPack data_pack (str.size(), sizeof(char));
			std::memcpy(data_pack.buffer, str.data(), str.size() * sizeof(char));

			std::vector<DataPack> import_data_pack = comm.gather(data_pack, MPI_CHAR, root);

			std::vector<T> import_data;
			for(std::size_t i = 0; i < import_data_pack.size(); i++) {
				auto item = import_data_pack[i];
				std::string import = std::string((char*) item.buffer, item.count);
				FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Gathered JSON from %i : %s", i, import.c_str());
				import_data.push_back(nlohmann::json::parse(import).get<T>());
			}
			return import_data;
		}

	template<typename T> T
		TypedMpi<T>::bcast(const T& data, int root) {
			std::string str = nlohmann::json(data).dump();
			FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Bcast JSON to root %i : %s", root, str.c_str());

			DataPack data_pack (str.size(), sizeof(char));
			std::memcpy(data_pack.buffer, str.data(), str.size() * sizeof(char));

			DataPack recv_data = comm.bcast(data_pack, MPI_CHAR, root);
			std::string recv = std::string((char*) recv_data.buffer, recv_data.count);

			return nlohmann::json::parse(recv).get<T>();
		}

	template<typename T>
		void TypedMpi<T>::send(const T& data, int destination, int tag) {
			std::string str = nlohmann::json(data).dump();
			FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Send JSON to process %i : %s", destination, str.c_str());
			comm.send(str.c_str(), str.size()+1, MPI_CHAR, destination, tag);
		}
	template<typename T>
		void TypedMpi<T>::Issend(const T& data, int destination, int tag, Request& req) {
			std::string str = nlohmann::json(data).dump();
			FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Issend JSON to process %i : %s", destination, str.c_str());
			comm.Issend(str.c_str(), str.size(), MPI_CHAR, destination, tag, req);
		}

	template<typename T>
		void TypedMpi<T>::probe(int source, int tag, Status &status) {
			return comm.probe(MPI_CHAR, source, tag, status);
		}
	template<typename T>
		bool TypedMpi<T>::Iprobe(int source, int tag, Status &status) {
			return comm.Iprobe(MPI_CHAR, source, tag, status);
		}

	template<typename T>
		T TypedMpi<T>::recv(int source, int tag, Status& status) {
			Status message_to_receive_status;
			this->probe(source, tag, message_to_receive_status);
			int count = message_to_receive_status.item_count;
			//MPI_Get_count(&message_to_receive_status, MPI_CHAR, &count);
			char* buffer = (char*) std::malloc(message_to_receive_status.size);
			comm.recv(buffer, count, MPI_CHAR, source, tag, status);

			std::string data (buffer, count);
			FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Receive JSON from process %i : %s", source, data.c_str());
			std::free(buffer);
			return nlohmann::json::parse(data).get<T>();
		}
}
}
#endif
