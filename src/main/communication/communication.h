#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <mpi.h>

namespace FPMAS {
	namespace communication {
		/**
		 * A convenient wrapper to build MPI groups and communicators from the
		 * MPI_COMM_WORLD communicator, i.e. groups and communicators
		 * containing all the processes available.
		 */
		class MpiCommunicator {
			private:
				int size;
				int rank;

				MPI_Group group;
				MPI_Comm comm;

			public:
				MpiCommunicator();
				MpiCommunicator(std::initializer_list<int>);
				MPI_Group getMpiGroup();
				MPI_Comm getMpiComm();

				int getRank();
				int getSize();

		};
	}
}
#endif
