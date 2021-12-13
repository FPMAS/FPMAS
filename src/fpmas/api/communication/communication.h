#ifndef FPMAS_COMMUNICATION_API_H
#define FPMAS_COMMUNICATION_API_H

/** \file src/fpmas/api/communication/communication.h
 * Communication API
 */

#include "fpmas/api/graph/distributed_id.h"

namespace fpmas { namespace api { namespace communication {


	/**
	 * A convenient wrapper for `void*` buffers used in MPI functions.
	 */
	class DataPack {
		public:
			/**
			 * Size of the buffer in bytes.
			 *
			 * `size = count * data_size`
			 */
			std::size_t size;
			/**
			 * Items count in the buffer.
			 */
			std::size_t count;
			/**
			 * Pointer to the internal buffer.
			 */
			char* buffer;

			/**
			 * Allocates a buffer of `count` items of size `data_size`.
			 */
			DataPack(std::size_t count, std::size_t data_size)
				: size(data_size*count), count(count) {
				buffer = (char*) std::malloc(size);
			}

			/**
			 * Allocates a void buffer, of size 0.
			 */
			DataPack() : DataPack(0, 0) {
			}

			/**
			 * Copy constructor.
			 *
			 * Allocates a buffer of the same size as `other`, and copies data
			 * from `other`'s buffer to `this` buffer.
			 *
			 * @param other DataPack to copy from
			 */
			DataPack(const DataPack& other)
				: size(other.size), count(other.count) {
					buffer = (char*) std::malloc(size);
					std::memcpy(buffer, other.buffer, size);
				}

			/**
			 * Move constructor.
			 *
			 * Moves `other's buffer to `this` buffer.
			 *
			 * @param other DataPack to move from
			 */
			DataPack(DataPack&& other)
				: size(other.size), count(other.count) {
					buffer = other.buffer;
					other.buffer = (char*) std::malloc(0);
					other.size = 0;
					other.count = 0;
				}

			/**
			 * Frees the internal buffer.
			 */
			void free() {
				std::free(buffer);
				// A buffer of size 0 is assigned to prevent issues when
				// std::free() is called again in the DataPack destructor.
				buffer = (char*) std::malloc(0);
				size = 0;
				count = 0;
			}

			/**
			 * Copy assignment.
			 *
			 * Frees `this` buffer, and copies `other`'s buffer into
			 * `this` buffer.
			 *
			 * @param other DataPack to copy from
			 */
			DataPack& operator=(const DataPack& other) {
				size = other.size;
				count = other.count;
				buffer = (char*) std::realloc(buffer, size);
				std::memcpy(buffer, other.buffer, size);
				return *this;
			}

			/**
			 * Move assignment.
			 *
			 * Frees `this` buffer, and moves `other`'s buffer into
			 * `this` buffer.
			 *
			 * @param other DataPack to move from
			 */
			DataPack& operator=(DataPack&& other) {
				std::free(buffer);
				size = other.size;
				count = other.count;
				buffer = other.buffer;
				other.buffer = (char*) std::malloc(0);
				other.size = 0;
				other.count = 0;
				return *this;
			}

			/**
			 * Frees the internal buffer.
			 */
			~DataPack() {
				std::free(buffer);
			}
	};

	/**
	 * Request type used in non-blocking communications.
	 *
	 * A request object stores a buffer containing the raw data that will be
	 * send by the concrete MPI operation, until it completes.
	 */
	struct Request {
		/**
		 * Low-level MPI_Request.
		 */
		MPI_Request __mpi_request;
		/**
		 * Data buffer.
		 */
		DataPack* __data = nullptr;

		Request() {}
		Request(const Request&) = delete;
		/**
		 * Request move constructor.
		 *
		 * @param request request to move
		 */
		Request(Request&& request) {
			this->__mpi_request = request.__mpi_request;
			this->__data = request.__data;
			request.__data = nullptr;
		}

		Request& operator=(const Request&) = delete;
		/**
		 * Request move assignment operator.
		 *
		 * @param request request to move
		 */
		Request& operator=(Request&& request) {
			this->__mpi_request = request.__mpi_request;
			this->__data = request.__data;
			request.__data = nullptr;
			return *this;
		}

		/**
		 * Frees the data buffer.
		 *
		 * Can be safely called even if the buffer is null or has already been
		 * freed.
		 */
		void free() {
			if(__data!=nullptr) {
				delete __data;
				__data = nullptr;
			}
		}
	};

	/**
	 * Status type, used to return information about messages.
	 */
	struct Status {
		/**
		 * A special default status that can be used if the user does not need to
		 * return status.
		 */
		static Status IGNORE;
		/**
		 * Message buffer size, in bytes.
		 */
		int size;
		/**
		 * Item count in the message buffer.
		 */
		int item_count;
		/**
		 * Message source.
		 */
		int source;
		/**
		 * Message tag.
		 */
		int tag;

		Status() : size(0), item_count(0), source(0), tag(0) {}
	};

	/**
	 * MpiCommunicator interface.
	 *
	 * Defines a low-level wrapper around MPI functions to make it more C++
	 * compliant.
	 *
	 * A limited set of functions, that are actually used in the `fpmas`
	 * library, have been adapted. More functions might be added in the future.
	 *
	 */
	class MpiCommunicator {
		public:
			/**
			 * Datatype used for recv / send operation without data.
			 *
			 * @see send(int, int, int)
			 * @see recv(int, int, int)
			 */
			static MPI_Datatype IGNORE_TYPE;

			/**
			 * Returns the rank of this communicator.
			 * @return rank
			 */
			virtual int getRank() const = 0;
			/**
			 * Returns the size of the group to which this communicator
			 * belongs.
			 * @return group size
			 */
			virtual int getSize() const = 0;

			/**
			 * Sends `data` to `destination` (blocking, asynchronous).
			 *
			 * @param data input buffer
			 * @param count items count in the buffer
			 * @param datatype MPI type of the data to send
			 * @param destination rank of the destination process
			 * @param tag message tag
			 */
			virtual void send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) = 0;

			/**
			 * Equivalent to \ref send(const void*, int, MPI_Datatype, int, int) "send(data.buffer, data.count, datatype, destination, tag)"
			 *
			 * @param data input DataPack
			 * @param datatype MPI type of the data to send
			 * @param destination rank of the destination process
			 * @param tag message tag
			 */
			virtual void send(const DataPack& data, MPI_Datatype datatype, int destination, int tag) = 0;

			/**
			 * Sends a void message to `destination` (blocking, asynchronous).
			 *
			 * This might be useful to send END message for example, when only
			 * a message tag without body might be enough.
			 *
			 * @param destination rank of the destination process
			 * @param tag message tag
			 */
			virtual void send(int destination, int tag) = 0;

			/**
			 * Sends `data` to `destination` (non-blocking, synchronous).
			 *
			 * Contrary to the default MPI specification, the specified `data`
			 * buffer can be reused immediately when the method return. This is
			 * allowed because the specified data is copied to the input
			 * `request`.
			 *
			 * In consequence, the Request **must** be completed using either
			 * test(Request&) or wait(Request&) to properly free the data
			 * buffer with the specified `request`.
			 *
			 * The request completes when it is guaranteed that the message has
			 * been received by the destination process.
			 *
			 *
			 * @param data input buffer
			 * @param count items count in the buffer
			 * @param datatype MPI type of the data to send
			 * @param destination rank of the destination process
			 * @param tag message tag
			 * @param request output Request
			 */
			virtual void Issend(
					const void* data, int count, MPI_Datatype datatype, int destination, int tag, Request& request) = 0;

			/**
			 * Equivalent to \ref Issend(const void*, int, MPI_Datatype, int, int, Request&) "Issend(data.buffer, data.count, datatype, destination, tag, request)".
			 *
			 * @param data input DataPack
			 * @param datatype MPI type of the data to send
			 * @param destination rank of the destination process
			 * @param tag message tag
			 * @param request output Request
			 */
			virtual void Issend(
					const DataPack& data, MPI_Datatype datatype, int destination, int tag, Request& request) = 0;

			/**
			 * Sends a void message to `destination` (non-blocking, synchronous).
			 *
			 * This might be useful to send notification messages when only a
			 * message tag without body might be enough.
			 *
			 * The test(Request&) or wait(Request&) functions can be used to wait for
			 * completion.
			 *
			 * The request completes when it is guaranteed that the message has
			 * been received by the destination process.
			 *
			 * @param destination rank of the destination process
			 * @param tag message tag
			 * @param request output Request
			 */
			virtual void Issend(int destination, int tag, Request& request) = 0;

			/**
			 * Blocking probe.
			 *
			 * Blocks until a message with the specified tag is received from
			 * source.
			 *
			 * Upon return, the output `status` contains information about the
			 * message to receive.
			 *
			 * @param type expected message type
			 * @param source source rank
			 * @param tag recv tag
			 * @param status MPI status
			 */
			virtual void probe(MPI_Datatype type, int source, int tag, Status& status) = 0;

			/**
			 * Non-blocking probe.
			 *
			 * Returns true if and only if a message with the specified tag can be received
			 * from source.
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
			virtual bool Iprobe(MPI_Datatype type, int source, int tag, Status& status) = 0;


			/**
			 * Receives a message without data.
			 *
			 * Completes an eventually corresponding synchronous send operation.
			 *
			 * @param source rank of the process to receive from
			 * @param tag message tag
			 * @param status output MPI status
			 */
			virtual void recv(int source, int tag, Status& status = Status::IGNORE) = 0;


			/**
			 * Receives data from `source` into `buffer`.
			 *
			 * The length of the output `buffer` should be large enough to
			 * contain the message to receive. probe() and Iprobe() might be
			 * use to query the length of a message before receiving it.
			 *
			 * @param buffer output buffer
			 * @param count items count to receive
			 * @param datatype MPI type of the data to receive
			 * @param source rank of the process to receive from
			 * @param tag message tag
			 * @param status output MPI status
			 */
			virtual void recv(void* buffer, int count, MPI_Datatype datatype, int source, int tag, Status& status) = 0;

			/**
			 * Equivalent to \ref recv(void*, int, MPI_Datatype, int, int, Status&) "recv(data.buffer, data.count, datatype, source, tag, status)"
			 *
			 * @param data output DataPack
			 * @param datatype MPI type of the data to receive
			 * @param source rank of the process to receive from
			 * @param tag message tag
			 * @param status output MPI status
			 */
			virtual void recv(DataPack& data, MPI_Datatype datatype, int source, int tag, Status& status) = 0;

			/**
			 * Tests if the input `request` is complete.
			 *
			 * Might be use to check the status of requests performed using
			 * non-blocking communication methods.
			 *
			 * Moreover, the `request` internal data buffer is freed if
			 * completion is detected.
			 *
			 * @param request Request to test
			 * @returns true iff the request is complete
			 */
			virtual bool test(Request& request) = 0;

			/**
			 * Waits for the input `request` to complete.
			 *
			 * Can be used to complete non-blocking communications. Upon
			 * return, the `request` internal data buffer is freed.
			 *
			 * @param request Request to test
			 */
			virtual void wait(Request& request) = 0;

			/**
			 * Performs a complete data exchange among processor.
			 *
			 * The `export_map` is a `[rank, data]` map : each DataPack is sent
			 * to the corresponding rank. The map is not required to contain an
			 * entry (i.e. data to send) for all available ranks.
			 *
			 * From each process, received data is gathered by origin rank in the
			 * returned `[rank, data]` map.
			 *
			 * @param export_map data to export to each process
			 * @param datatype MPI type of the data to send / receive
			 * @return data received from each process
			 */
			// TODO: should std::vector be used instead of map? pb: what if we
			// want to send "no data" to process?
			virtual std::unordered_map<int, DataPack>
				allToAll(std::unordered_map<int, DataPack> export_map, MPI_Datatype datatype) = 0;

			/**
			 * Gathers data at `root`.
			 *
			 * All processes (including `root`), specify `data` to send to `root`.
			 * If the current process rank corresponds to `root`, sent data,
			 * ordered by rank, are returned in the output vector. Else, an empty
			 * vector is returned.
			 *
			 * Notice that input DataPacks might have different length, and can
			 * be empty for some processes.
			 *
			 * @param data data to send to root
			 * @param datatype MPI datatype
			 * @param root rank of the root process
			 * @return if `rank == root`, a vector containing gathered data,
			 * else an empty vector.
			 */
			virtual std::vector<DataPack>
				gather(DataPack data, MPI_Datatype datatype, int root) = 0;

			/**
			 * Gathers data on all processes.
			 *
			 * Each process sends a `data` instance, and all sent data are
			 * received on all processes, ordered by rank. In consequence,
			 * vectors returned on all processes are the same.
			 *
			 * Notice that input DataPacks might have different length, and can
			 * be empty for some processes.
			 *
			 * @param data to send to all processes
			 * @param datatype MPI datatype
			 * @return data sent by all processes
			 */
			virtual std::vector<DataPack>
				allGather(DataPack data, MPI_Datatype datatype) = 0;

			/**
			 * Broadcasts data.
			 *
			 * `data` is sent from `root` to all other processes, and received
			 * data is returned (including at `root`). On other processes than
			 * root, the `data` parameter is ignored.
			 *
			 * Notice that the input DataPack might be empty.
			 *
			 * @param data data to broadcast
			 * @param datatype MPI datatype
			 * @param root rank of the process from which data is sent
			 * @return received data from `root`
			 */
			virtual DataPack bcast(DataPack data, MPI_Datatype datatype, int root) = 0;

			/**
			 * Defines a synchronization barrier.
			 *
			 * Blocks the caller until all processes have called it. The
			 * call returns at any process only after all processes have
			 * entered the call.
			 */
			virtual void barrier() = 0;

			virtual ~MpiCommunicator() {};
	};

	/**
	 * A conveniently templated MPI wrapper to handle any type of data.
	 *
	 * Provides an intuitive API to send / receive objects of type T through
	 * MPI, without having to manually serialize / unserialize data and specify
	 * send / receive buffers.
	 *
	 * Of course, serialization / unserialization rules must still be specified
	 * at the implementation level.
	 *
	 * For example, implementations might be based on a JSON or XML
	 * serialization of T, on custom MPI Datatypes, or on any other custom protocol.
	 */
	template<typename T>
		class TypedMpi {
			public:
				/**
				 * Performs a migration of data across all processors.
				 *
				 * The "migration" operation follows the same rules as
				 * MpiCommunicator::allToAll, except that vectors of T instances
				 * can be used, for convenience.
				 *
				 * @param export_map vectors of data to send to each process
				 * @return vectors of data received from each process
				 */
				// TODO: Remove this in 2.0
				virtual std::unordered_map<int, std::vector<T>>
					migrate(std::unordered_map<int, std::vector<T>> export_map) = 0;

				/**
				 * Performs a complete exchange of `T` instances among processor.
				 *
				 * Follows the same rules as MpiCommunicator::allToAll().
				 *
				 * @param export_map data to export to each process
				 * @return data received from each process
				 */
				virtual std::unordered_map<int, T>
					allToAll(std::unordered_map<int, T> export_map) = 0;

				/**
				 * Gathers `T` instances at `root`.
				 *
				 * Follows the same rules as MpiCommunicator::gather().
				 *
				 * @param data data to send to root
				 * @param root rank of the root process
				 * @return if `rank == root`, a vector containing gathered `T`
				 * objects, else an empty vector.
				 */
				virtual std::vector<T>
					gather(const T& data, int root) = 0;

				/**
				 * Gathers `T` instances on all processes.
				 *
				 * Follows the same rules as MpiCommunicator::allGather().
				 *
				 * @param data to send to all processes
				 * @return data sent by all processes
				 */
				virtual std::vector<T>
					allGather(const T& data) = 0;

				/**
				 * Broadcasts a `T` instance to all processes.
				 *
				 * Follows the same rules as MpiCommunicator::bcast().
				 *
				 * @param data data to broadcast
				 * @param root rank of the process from which data is sent
				 * @return received data from `root`
				 */
				virtual T bcast(const T& data, int root) = 0;

				/**
				 * Sends a `T` instance to `destination`.
				 *
				 * Follows the same rules as MpiCommunicator::send().
				 *
				 * @param data reference to the `T` object to send to `destination`
				 * @param destination rank of the destination process
				 * @param tag message tag
				 */
				virtual void send(const T& data, int destination, int tag) = 0;

				/**
				 * Sends a `T` instance to `destination`.
				 *
				 * Follows the same rules as MpiCommunicator::Issend().
				 *
				 * @param data reference to the `T` object to send to `destination`
				 * @param destination rank of the destination process
				 * @param tag message tag
				 * @param req output Request
				 */
				virtual void Issend(const T& data, int destination, int tag, Request& req) = 0;

				/**
				 * Blocking probe.
				 *
				 * Blocks until a message with the specified tag is received from
				 * source.
				 *
				 * Upon return, the output `status` contains information about the
				 * size of the message to receive.
				 *
				 * @param source source rank
				 * @param tag recv tag
				 * @param status MPI status
				 */
				virtual void probe(int source, int tag, Status& status) = 0;

				/**
				 * Non-blocking probe.
				 *
				 * Returns true if and only if a message with the specified tag can be received
				 * from source.
				 *
				 * Upon return, the output `status` contains information about the
				 * size of the message to receive.
				 *
				 * @param source source rank
				 * @param tag recv tag
				 * @param status MPI status
				 * @return true iff a message is available
				 */
				virtual bool Iprobe(int source, int tag, Status& status) = 0;

				/**
				 * Receives a `T` object from `source`.
				 *
				 * @param source rank of the process to receive from
				 * @param tag message tag
				 * @param status output MPI status
				 */
				virtual T recv(int source, int tag, Status& status = Status::IGNORE) = 0;

				virtual ~TypedMpi() {};
		};

	/**
	 * Utility function used to compare DataPacks.
	 */
	bool operator==(const DataPack& d1, const DataPack& d2);
}
}}
#endif
