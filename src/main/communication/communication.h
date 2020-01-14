#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <string>
#include <mpi.h>

namespace FPMAS {
	namespace communication {
		enum State {
			ACTIVE,
			PASSIVE
		};

		enum Tag : int {
			READ,
			ACQUIRE,
			TOKEN,
			END
		};

		enum Color : int {
			WHITE = 0,
			BLACK = 1
		};

		/**
		 * A convenient wrapper to build MPI groups and communicators from the
		 * MPI_COMM_WORLD communicator, i.e. groups and communicators
		 * containing all the processes available.
		 */
		class MpiCommunicator {
			private:
				int size;
				int rank;
				State state = State::ACTIVE;
				Color color = Color::WHITE;

				MPI_Group group;
				MPI_Comm comm;

				void send(std::string, int, Tag);

			public:
				MpiCommunicator();
				MpiCommunicator(std::initializer_list<int>);
				MPI_Group getMpiGroup();
				MPI_Comm getMpiComm();

				int getRank() const;
				int getSize() const;
				State getState() const;

				void terminate();


		};
	}
}
#endif
