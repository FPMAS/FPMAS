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
#include "fpmas/io/json_datapack.h"

namespace fpmas {
	void init(int argc, char** argv);
}

namespace fpmas { namespace communication {
	using api::communication::DataPack;
	using api::communication::Request;
	using api::communication::Status;

	/**
	 * fpmas::api::communication::MpiCommunicator implementation, based on
	 * the system MPI library (i.e. `#include <mpi.h>`).
	 */
	class MpiCommunicatorBase : public virtual api::communication::MpiCommunicator {
		protected:
			/**
			 * Communicator size
			 */
			int size;
			/**
			 * Communicator rank
			 */
			int rank;

			/**
			 * Internal MPI_Group
			 */
			MPI_Group group;
			/**
			 * Internal MPI_Comm
			 */
			MPI_Comm comm;

		private:
			static void convertStatus(MPI_Status&, Status&, MPI_Datatype datatype);

		public:
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
			void send(
					const void* data, int count, MPI_Datatype datatype,
					int destination, int tag) override;

			void send(
					const DataPack& data, MPI_Datatype datatype,
					int destination, int tag) override;

			/**
			 * Performs an MPI_Send operation without data.
			 *
			 * @param destination rank of the destination proc
			 * @param tag message tag
			 */
			void send(int destination, int tag) override;


			void Isend(
					const void* data, int count, MPI_Datatype datatype,
					int destination, int tag, Request& req) override;

			void Isend(
					const DataPack& data, MPI_Datatype datatype,
					int destination, int tag, Request& req) override;

			void Isend(int destination, int tag, Request& req) override;

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
					const void* data, int count, MPI_Datatype datatype,
					int destination, int tag, Request& req) override;

			void Issend(
					const DataPack& data, MPI_Datatype datatype,
					int destination, int tag, Request& req) override;
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
			void recv(
					void* buffer, int count, MPI_Datatype datatype,
					int source, int tag, Status& status = Status::IGNORE) override;

			void recv(
					DataPack& data, MPI_Datatype datatype,
					int source, int tag, Status& status = Status::IGNORE) override;

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

			/**
			 * Performs an MPI_Wait operation.
			 *
			 * @param req Request to wait for completion
			 */
			void wait(Request& req) override;

			void waitAll(std::vector<Request>& req) override;

			/**
			 * Performs an MPI_Alltoall operation.
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
			 * @param data data to send to root
			 * @param datatype MPI datatype
			 * @param root rank of the root process
			 * @return if `rank == root`, a vector containing gathered data,
			 * else an empty vector.
			 */
			std::vector<DataPack>
				gather(DataPack data, MPI_Datatype datatype, int root) override;

			/**
			 * Performs an MPI_Allgather operation.
			 *
			 * @param data to send to all processes
			 * @param datatype MPI datatype
			 * @return data sent by all processes
			 */
			std::vector<DataPack>
				allGather(DataPack data, MPI_Datatype datatype) override;

			/**
			 * Performs an MPI_Bcast operation.
			 *
			 * @param data data to broadcast
			 * @param datatype MPI datatype
			 * @param root rank of the process from which data is sent
			 * @return received data from `root`
			 */
			DataPack bcast(DataPack data, MPI_Datatype datatype, int root) override;
			
			/**
			 * Performs an MPI_Barrier operation.
			 */
			void barrier() override;
	};


	/**
	 * api::communication::MpiCommunicator implementation.
	 */
	class MpiCommunicator : public MpiCommunicatorBase {
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
			 * MpiCommunicator destructor.
			 *
			 * Frees allocated MPI resources.
			 */
			~MpiCommunicator();
	};

	/**
	 * Special api::communication::MpiCommunicator implementation, built from
	 * MPI_COMM_WORLD.
	 */
	class MpiCommWorld : public MpiCommunicatorBase {
		public:
		/**
		 * Initializes internal MPI structures from MPI_COMM_WORLD.
		 *
		 * This must be called **after** MPI_Init(). Since an MpiCommWorld
		 * instance is very likely to be declared static (see WORLD), this
		 * can't be performed in the constructor, since MPI_Init() can't be
		 * called before the call to a constructor of a static variable.
		 */
		void init() {
			this->comm = MPI_COMM_WORLD;
			MPI_Comm_group(MPI_COMM_WORLD, &this->group);
			MPI_Comm_size(MPI_COMM_WORLD, &this->size);
			MPI_Comm_rank(MPI_COMM_WORLD, &this->rank);
		};
	};

	/**
	 * MpiCommWorld instance, initialized by fpmas::init().
	 */
	extern MpiCommWorld WORLD;

	namespace detail {
		/**
		 * An fpmas::io::datapack::BasicObjectPack based
		 * fpmas::api::communication::TypedMpi implementation.
		 *
		 * Each `T` instance is serialized as a DataPack using the specified
		 * PackType, and sent as MPI_CHAR using the provided
		 * api::communication::MpiCommunicator.
		 *
		 * This means that ANY TYPE that can be serialized / unserialized as
		 * PackType can be easily sent across processors through MPI using this
		 * class, preventing users from struggling with low-level MPI issues
		 * and custom MPI_Datatype definitions.
		 *
		 *
		 * @tparam T data to transmit, serializable into a `PackType`
		 * @tparam PackType BasicObjectPack implementation (e.g.:
		 * fpmas::io::datapack::JsonPack, fpmas::io::datapack::ObjectPack,
		 * fpmas::io::datapack::LightObjectPack...)
		 */
		template<typename T, typename PackType>
			class TypedMpi : public api::communication::TypedMpi<T> {
				private:
					api::communication::MpiCommunicator& comm;
				public:
					/**
					 * BasicObjectPack implementation used to serialize data.
					 */
					typedef PackType pack_type;

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

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::migrate
					 */
					std::unordered_map<int, std::vector<T>>
						migrate(std::unordered_map<int, std::vector<T>> export_map) override;

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::allToAll
					 */
					std::unordered_map<int, T>
						allToAll(std::unordered_map<int, T> export_map) override;

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::gather
					 */
					std::vector<T> gather(const T&, int root) override;
					/**
					 * \copydoc fpmas::api::communication::TypedMpi::allGather
					 */
					std::vector<T> allGather(const T&) override;
					/**
					 * \copydoc fpmas::api::communication::TypedMpi::bcast
					 */
					T bcast(const T&, int root) override;

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::send
					 */
					void send(const T&, int, int) override;

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::Isend
					 */
					void Isend(const T&, int, int, Request&) override;

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::Issend
					 */
					void Issend(const T&, int, int, Request&) override;

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::probe
					 */
					void probe(int source, int tag, Status& status) override;
					/**
					 * \copydoc fpmas::api::communication::TypedMpi::Iprobe
					 */
					bool Iprobe(int source, int tag, Status& status) override;

					/**
					 * \copydoc fpmas::api::communication::TypedMpi::recv
					 */
					T recv(int source, int tag, Status& status = Status::IGNORE) override;
			};

		template<typename T, typename PackType> std::unordered_map<int, std::vector<T>>
			TypedMpi<T, PackType>::migrate(std::unordered_map<int, std::vector<T>> export_map) {
				std::unordered_map<int, DataPack> import_data_pack;

				{
					// Pack
					std::unordered_map<int, DataPack> export_data_pack;
					for(auto& item : export_map)
						export_data_pack.emplace(item.first, PackType(item.second).dump());

					// export_data_pack buffers are moved to the temporary allToAll
					// argument, and automatically freed by the allToAll
					// implementation
					import_data_pack = comm.allToAll(
							std::move(export_data_pack), MPI_CHAR
							);
				}

				std::unordered_map<int, std::vector<T>> import_map;
				for(auto& item : import_data_pack)
					import_map.emplace(
							item.first, PackType::parse(std::move(item.second))
							.template get<std::vector<T>>()
							);
				
				// Should perform "copy elision"
				return import_map;
			}

		template<typename T, typename PackType> std::unordered_map<int, T>
			TypedMpi<T, PackType>::allToAll(std::unordered_map<int, T> export_map) {
				// Pack
				std::unordered_map<int, DataPack> import_data_pack;

				{
					std::unordered_map<int, DataPack> export_data_pack;
					for(auto& item : export_map)
						export_data_pack.emplace(item.first, PackType(item.second).dump());

					// export_data_pack buffers are moved to the temporary allToAll
					// argument, and automatically freed by the allToAll
					// implementation
					import_data_pack = comm.allToAll(
							std::move(export_data_pack), MPI_CHAR
							);
				}

				std::unordered_map<int, T> import_map;
				for(auto& item : import_data_pack)
					import_map.emplace(
							item.first, PackType::parse(std::move(item.second))
							.template get<T>()
							);

				// Should perform "copy elision"
				return import_map;
			}

		template<typename T, typename PackType> std::vector<T>
			TypedMpi<T, PackType>::gather(const T& data, int root) {
				DataPack data_pack = PackType(data).dump();

				std::vector<DataPack> import_data_pack
					= comm.gather(data_pack, MPI_CHAR, root);

				std::vector<T> import_data;
				for(std::size_t i = 0; i < import_data_pack.size(); i++) {
					import_data.emplace_back(
							PackType::parse(
								std::move(import_data_pack[i])).template get<T>()
							);
				}
				return import_data;
			}

		template<typename T, typename PackType> std::vector<T>
			TypedMpi<T, PackType>::allGather(const T& data) {
				// Pack
				DataPack data_pack = PackType(data).dump();

				std::vector<DataPack> import_data_pack
					= comm.allGather(data_pack, MPI_CHAR);

				std::vector<T> import_data;
				for(std::size_t i = 0; i < import_data_pack.size(); i++) {
					import_data.emplace_back(
							PackType::parse(
								std::move(import_data_pack[i])).template get<T>()
							);
				}
				return import_data;
			}

		template<typename T, typename PackType>
			T TypedMpi<T, PackType>::bcast(const T& data, int root) {
				DataPack data_pack = PackType(data).dump();

				DataPack recv_data_pack = comm.bcast(data_pack, MPI_CHAR, root);

				return PackType::parse(std::move(recv_data_pack)).template get<T>();
			}

		template<typename T, typename PackType>
			void TypedMpi<T, PackType>::send(const T& data, int destination, int tag) {
				//FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Send JSON to process %i : %s", destination, str.c_str());
				DataPack data_pack = PackType(data).dump();
				comm.send(data_pack, MPI_CHAR, destination, tag);
			}

		template<typename T, typename PackType>
			void TypedMpi<T, PackType>::Isend(const T& data, int destination, int tag, Request& req) {
				//FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Issend JSON to process %i : %s", destination, str.c_str());
				DataPack data_pack = PackType(data).dump();
				comm.Isend(data_pack, MPI_CHAR, destination, tag, req);
			}


		template<typename T, typename PackType>
			void TypedMpi<T, PackType>::Issend(const T& data, int destination, int tag, Request& req) {
				//FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Issend JSON to process %i : %s", destination, str.c_str());
				DataPack data_pack = PackType(data).dump();
				comm.Issend(data_pack, MPI_CHAR, destination, tag, req);
			}

		template<typename T, typename PackType>
			void TypedMpi<T, PackType>::probe(int source, int tag, Status &status) {
				return comm.probe(MPI_CHAR, source, tag, status);
			}
		template<typename T, typename PackType>
			bool TypedMpi<T, PackType>::Iprobe(int source, int tag, Status &status) {
				return comm.Iprobe(MPI_CHAR, source, tag, status);
			}

		template<typename T, typename PackType>
			T TypedMpi<T, PackType>::recv(int source, int tag, Status& status) {
				Status message_to_receive_status;
				this->probe(source, tag, message_to_receive_status);
				int count = message_to_receive_status.item_count;
				DataPack data_pack(count, sizeof(char));
				comm.recv(data_pack, MPI_CHAR, source, tag, status);

				//FPMAS_LOGD(comm.getRank(), "TYPED_MPI", "Receive JSON from process %i : %s", source, data.c_str());
				return PackType::parse(std::move(data_pack)).template get<T>();
			}
	}

	/**
	 * The default fpmas::communication::detail::TypedMpi specialization, based
	 * on io::datapack::ObjectPack.
	 *
	 * Since ObjectPack serialization rules are predefined for fundamental types
	 * and std containers, sending such structure through MPI is made trivial,
	 * without any additionnal user defined code:
	 * ```cpp
	 * // Used to transmit integers: predefined
	 * TypedMpi<int> int_mpi;
	 * // Used to transmit vectors of integers: predefined
	 * TypedMpi<std::vector<int>> int_vector_mpi;
	 * ```
	 * If a `CustomType` is serializable in an `ObjectPack`, containers of this
	 * type can trivially be transmitted, without any additionnal code:
	 * ```cpp
	 * TypedMpi<std::vector<CustomType>> custom_type_vector_mpi;
	 * ```
	 *
	 * @tparam T type to transmit, serializable into an
	 * fpmas::io::datapack::ObjectPack
	 */
	template<typename T>
		struct TypedMpi : public detail::TypedMpi<T, io::datapack::ObjectPack> {
			using detail::TypedMpi<T, io::datapack::ObjectPack>::TypedMpi;
		};


	/**
	 * Gathers `data` at `root` using the provided `mpi` instance, and returns
	 * the accumulated result.
	 *
	 * `binary_op` is the operation used to accumulate data of the vector
	 * returned by the gather operation.
	 *
	 * On processes other that `root`, `data` is returned.
	 *
	 * @param mpi mpi instance used to perform the gather operation
	 * @param root rank of the processes on which data should be gathered and
	 * accumulated
	 * @param data local data instance to gather
	 * @param binary_op operation used to accumulate data
	 *
	 * @see fpmas::api::communication::TypedMpi::gather()
	 * @see https://en.cppreference.com/w/cpp/algorithm/accumulate
	 */
	template<typename T, typename BinaryOp = std::plus<T>>
		T reduce(
				api::communication::TypedMpi<T>& mpi, int root,
				const T& data, BinaryOp binary_op = BinaryOp()) {
			std::vector<T> data_vec = mpi.gather(data, root);
			if(data_vec.size() > 0)
				return std::accumulate(std::next(data_vec.begin()), data_vec.end(), data_vec[0], binary_op);
			return data;
		}
	/**
	 * Gathers `data` using the provided `mpi` instance, and returns
	 * the accumulated result on all processes (using an
	 * fpmas::api::communication::TypedMpi::allGather() operation).
	 *
	 * `binary_op` is the operation used to accumulate data of the vector
	 * returned by the gather operation.
	 *
	 *
	 * @param mpi mpi instance used to perform the allGather() operation
	 * @param data local data instance to gather
	 * @param binary_op operation used to accumulate data
	 *
	 * @see fpmas::api::communication::TypedMpi::allGather()
	 * @see https://en.cppreference.com/w/cpp/algorithm/accumulate
	 */
	template<typename T, typename BinaryOp = std::plus<T>>
		T all_reduce(
				api::communication::TypedMpi<T>& mpi,
				const T& data, BinaryOp binary_op = BinaryOp()) {
			std::vector<T> data_vec = mpi.allGather(data);
			return std::accumulate(std::next(data_vec.begin()), data_vec.end(), data_vec[0], binary_op);
		}
}}
#endif
