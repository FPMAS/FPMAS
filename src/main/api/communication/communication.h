#ifndef MPI_COMMUNICATOR_API_H
#define MPI_COMMUNICATOR_API_H

#include "mpi.h"
#include "graph/parallel/distributed_id.h"

namespace FPMAS::api::communication {

	/**
	 * MpiCommunicator interface.
	 *
	 * Defines operations that might be used by the RequestHandler.
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

			
			virtual void send(const void* data, int count, MPI_Datatype datatype, int destination, int tag) = 0;
			virtual void send(int destination, int tag) = 0;

			virtual void Issend(
					const void* data, int count, MPI_Datatype datatype, int destination, int tag, MPI_Request* req) = 0;
			virtual void Issend(int destination, int tag, MPI_Request* req) = 0;
			
			/**
			 * Blocking probe.
			 *
			 * Blocks until a message with the specified tag is received from
			 * source.
			 *
			 * @param source source rank
			 * @param tag recv tag
			 * @param status MPI status
			 */
			virtual void probe(int source, int tag, MPI_Status* status) = 0;

			/**
			 * Non-blocking probe.
			 *
			 * Return 1 iff a message with the specified tag can be received
			 * from source.
			 *
			 * @param source source rank
			 * @param tag recv tag
			 * @param status MPI status
			 */
			virtual bool Iprobe(int source, int tag, MPI_Status* status) = 0;


			/**
			 * Receives a message without data.
			 *
			 * Completes an eventually synchronous send operation.
			 *
			 * status should have been passed to a previous probe request.
			 *
			 * @param status ready to receive MPI status
			 */
			virtual void recv(MPI_Status* status) = 0;

			virtual void recv(void* buffer, int count, MPI_Datatype datatype, int source, int tag, MPI_Status* status) = 0;

			virtual bool test(MPI_Request*) = 0;

			virtual std::unordered_map<int, std::string>
				allToAll(std::unordered_map<int, std::string>) = 0;

			virtual ~MpiCommunicator() {};
	};

	template<typename T>
	class TypedMpi {
		public:
			virtual std::unordered_map<int, std::vector<T>>
				migrate(std::unordered_map<int, std::vector<T>> exportMap) = 0;

			virtual void send(const T&, int destination, int tag) = 0;
			virtual void Issend(const T&, int destination, int tag, MPI_Request* req) = 0;
			virtual T recv(MPI_Status*) = 0;

			virtual ~TypedMpi() {};
	};

	template<typename MpiCommunicatorImpl, template<typename> class TypedMpiImpl>
		class MpiSetUp {
			static_assert(std::is_base_of<MpiCommunicator, MpiCommunicatorImpl>::value);
			static_assert(std::is_base_of<TypedMpi<int>, TypedMpiImpl<int>>::value);

			public:
			typedef MpiCommunicatorImpl communicator;
			template<typename T>
				using mpi = TypedMpiImpl<T>;
		};
}
#endif
