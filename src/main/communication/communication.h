#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <string>
#include <mpi.h>

#include "resource_manager.h"

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
			protected:
				int size;
				int rank;

				MPI_Group group;
				MPI_Comm comm;

			public:
				MpiCommunicator();
				MpiCommunicator(std::initializer_list<int>);
				MPI_Group getMpiGroup();
				MPI_Comm getMpiComm();

				int getRank() const;
				int getSize() const;

		};

		class TerminableMpiCommunicator : public MpiCommunicator {
			private:
				State state = State::ACTIVE;
				Color color = Color::WHITE;

				ResourceManager* resourceManager;

				void send(std::string, int, Tag);
				void respondToRead(int, unsigned long);

			public:
				TerminableMpiCommunicator(ResourceManager*);
				TerminableMpiCommunicator(ResourceManager*, std::initializer_list<int>);

				State getState() const;

				std::string read(unsigned long, int);
				std::string acquire(unsigned long, int);

				void terminate();

		};
	}
}
#endif
