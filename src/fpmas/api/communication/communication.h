#ifndef MPI_COMMUNICATOR_API_H
#define MPI_COMMUNICATOR_API_H

#include "mpi.h"
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
				int size;
				/**
				 * Items count in the buffer.
				 */
				int count;
				/**
				 * Pointer to the internal buffer.
				 */
				void* buffer;

				/**
				 * Allocates a buffer of `count` items of size `data_size`.
				 */
				DataPack(int count, size_t data_size) : size(data_size*count), count(count) {
					buffer = std::malloc(size);
				}

				/**
				 * Allocates a void buffer, of size 0.
				 */
				DataPack() : size(0), count(0) {
					buffer = std::malloc(0);
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
						buffer = std::malloc(size);
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
						other.buffer = std::malloc(0);
					}

				/**
				 * Copy assignment.
				 *
				 * Frees `this` buffer, and moves `other`'s buffer into
				 * `this` buffer.
				 *
				 * @param other DataPack to copy from
				 */
				DataPack& operator=(const DataPack& other) {
					std::free(buffer);
					size = other.size;
					count = other.count;
					buffer = std::malloc(size);
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
					other.buffer = std::malloc(0);
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
				 * @param destination rank of the destination proc
				 * @param tag message tag
				 */
				virtual void send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) = 0;

				/**
				 * Sends a void message to `destination` (blocking, asynchronous).
				 *
				 * This might be useful to send END message for example, when only
				 * a message tag without body might be enough.
				 *
				 * @param destination rank of the destination proc
				 * @param tag message tag
				 */
				virtual void send(int destination, int tag) = 0;

				/**
				 * Sends `data` to `destination` (non-blocking, synchronous).
				 *
				 * The test(MPI_Request*) function can be used to wait for
				 * completion, using the output `MPI_Request`.
				 *
				 * The request completes when it is guaranteed that the message has
				 * been received by the destination proc.
				 *
				 * @param data input buffer
				 * @param count items count in the buffer
				 * @param datatype MPI type of the data to send
				 * @param destination rank of the destination proc
				 * @param tag message tag
				 * @param req output MPI request
				 */
				virtual void Issend(
						const void* data, int count, MPI_Datatype datatype, int destination, int tag, MPI_Request* req) = 0;
				/**
				 * Sends a void message to `destination` (blocking).
				 *
				 * This might be useful to send END message for example, when only
				 * a message tag without body might be enough.
				 *
				 * The test(MPI_Request*) function can be used to wait for
				 * completion, using the output `MPI_Request`.
				 *
				 * The request completes when it is guaranteed that the message has
				 * been received by the destination proc.
				 *
				 * @param destination rank of the destination proc
				 * @param tag message tag
				 * @param req output MPI request
				 */
				virtual void Issend(int destination, int tag, MPI_Request* req) = 0;

				/**
				 * Blocking probe.
				 *
				 * Blocks until a message with the specified tag is received from
				 * source.
				 *
				 * Upon return, the output `status` contains information about the
				 * size of the message to receive. Such information can be decoded
				 * using `MPI_Get_count(const MPI_Status*, MPI_Datatype, int* count)`.
				 *
				 * @param source source rank
				 * @param tag recv tag
				 * @param status MPI status
				 */
				virtual void probe(int source, int tag, MPI_Status* status) = 0;

				/**
				 * Non-blocking probe.
				 *
				 * Returns true if and only if a message with the specified tag can be received
				 * from source.
				 *
				 * Upon return, the output `status` contains information about the
				 * size of the message to receive. Such information can be decoded
				 * using `MPI_Get_count(const MPI_Status*, MPI_Datatype, int* count)`.
				 *
				 * @param source source rank
				 * @param tag recv tag
				 * @param status MPI status
				 * @return true iff a message is available
				 */
				virtual bool Iprobe(int source, int tag, MPI_Status* status) = 0;


				/**
				 * Receives a message without data.
				 *
				 * Completes an eventually corresponding synchronous send operation.
				 *
				 * @param source rank of the process to receive from
				 * @param tag message tag
				 * @param status output MPI status
				 */
				virtual void recv(int source, int tag, MPI_Status* status = MPI_STATUS_IGNORE) = 0;


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
				 * @param source rank of the proc to receive from
				 * @param tag message tag
				 * @param status output MPI status
				 */
				virtual void recv(void* buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status) = 0;

				/**
				 * Tests if the input request is complete.
				 *
				 * Might be use to check the status of request performed using
				 * Issend().
				 *
				 * @param req MPI request to test
				 * @returns true iff the request is complete
				 */
				virtual bool test(MPI_Request* req) = 0;

				/**
				 * Performs a complete data exchange among processor.
				 *
				 * The `export_map` is a `[rank, data]` map : each DataPack is sent
				 * to the corresponding rank. The map is not required to contain an
				 * entry (i.e. data to send) for all available ranks.
				 *
				 * From each proc, received data is gathered by origin rank in the
				 * returned `[rank, data]` map.
				 *
				 * @param export_map data to export to each proc
				 * @param datatype MPI type of the data to send / receive
				 * @return data received from each proc
				 */
				virtual std::unordered_map<int, DataPack>
					allToAll(std::unordered_map<int, DataPack> export_map, MPI_Datatype datatype) = 0;

				/**
				 * Gathers data at `root`.
				 *
				 * All procs (including `root`), specify `data` to send to `root`.
				 * If the current process rank corresponds to `root`, sent data,
				 * ordered by rank, are returned in the output vector. Else, an empty
				 * vector is returned.
				 *
				 * Notice that input DataPacks might have different length, and can
				 * be empty for some procs.
				 *
				 * @param data data to send to root
				 * @param root rank of the root process
				 * @return if `rank == root`, a vector containing gathered data,
				 * else an empty vector.
				 */
				virtual std::vector<DataPack>
					gather(DataPack data, MPI_Datatype datatype, int root) = 0;

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
					 * @param export_map vectors of data to send to each proc
					 * @return vectors of data received from each proc
					 */
					virtual std::unordered_map<int, std::vector<T>>
						migrate(std::unordered_map<int, std::vector<T>> export_map) = 0;

					/**
					 * Gathers `T` objects at `root`.
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
					 * Sends a `T` object to `destination`.
					 *
					 * Follows the same rules as MpiCommunicator::send().
					 *
					 * @param data reference to the `T` object to send to `destination`
					 * @param destination rank of the destination process
					 * @param tag message tag
					 */
					virtual void send(const T& data, int destination, int tag) = 0;
					/**
					 * Sends a `T` object to `destination`.
					 *
					 * Follows the same rules as MpiCommunicator::Issend().
					 *
					 * @param data reference to the `T` object to send to `destination`
					 * @param destination rank of the destination process
					 * @param tag message tag
					 * @param req output MPI request
					 */
					virtual void Issend(const T&, int destination, int tag, MPI_Request* req) = 0;
					/**
					 * Receives a `T` object from `source`.
					 *
					 * @param source rank of the process to receive from
					 * @param tag message tag
					 * @param status output MPI status
					 */
					virtual T recv(int source, int tag, MPI_Status* status = MPI_STATUS_IGNORE) = 0;

					virtual ~TypedMpi() {};
			};

		template<typename MpiCommunicatorImpl, template<typename> class TypedMpiImpl>
			class MpiSetUp {
				//static_assert(std::is_base_of<MpiCommunicator, MpiCommunicatorImpl>::value);
				//static_assert(std::is_base_of<TypedMpi<int>, TypedMpiImpl<int>>::value);

				public:
				typedef MpiCommunicatorImpl communicator;
				template<typename T>
					using mpi = TypedMpiImpl<T>;
			};

		/**
		 * Utility function used to compare DataPacks.
		 */
		inline bool operator ==(const fpmas::api::communication::DataPack& d1, const fpmas::api::communication::DataPack& d2) {
			if(d1.count != d2.count)
				return false;
			if(d1.size != d2.size)
				return false;
			for(int i = 0; i < d1.size; i++) {
				if(((char*) d1.buffer)[i] != ((char*) d2.buffer)[i])
					return false;
			}
			return true;
		}
	}
}}
#endif
