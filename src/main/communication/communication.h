#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <cstdint>
#include <string>
#include <mpi.h>
#include <unordered_map>

#include "resource_container.h"
#include "readers_writers.h"

namespace FPMAS {
	/**
	 * The FPMAS::communication namespace contains low level MPI features
	 * required by communication processes.
	 */
	namespace communication {
		enum State {
			ACTIVE,
			PASSIVE
		};

		enum Tag : int {
			READ,
			READ_RESPONSE,
			ACQUIRE,
			ACQUIRE_RESPONSE,
			ACQUIRE_GIVE_BACK,
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
				MpiCommunicator& operator=(MpiCommunicator);
				MpiCommunicator(std::initializer_list<int>);
				MPI_Group getMpiGroup() const;
				MPI_Comm getMpiComm() const;

				int getRank() const;
				int getSize() const;

				~MpiCommunicator();

		};

		/**
		 * An MpiCommunicator extension that implements the
		 * Dijkstra-Feijen-Gasteren termination algorithm.
		 *
		 * The user can invoke the terminate() function to synchronize the
		 * communicators.
		 *
		 */
		class TerminableMpiCommunicator : public MpiCommunicator {
			private:
				State state = State::ACTIVE;
				Color color = Color::WHITE;

				ResourceContainer& resourceContainer;
				ResourceManager resourceManager;
				

				void send(std::string, int, Tag);
				void waitSendRequest(MPI_Request*);

				void respondToRead(int, unsigned long);
				void respondToAcquire(int, unsigned long);

				void waitForReading(unsigned long);
				void waitForAcquire(unsigned long);

			public:
				TerminableMpiCommunicator(ResourceContainer&);
				TerminableMpiCommunicator(ResourceContainer&, std::initializer_list<int>);

				State getState() const;

				void handleIncomingRequests();

				std::string read(unsigned long, int);

				std::string acquire(unsigned long, int);
				void giveBack(unsigned long, int);

				void terminate();

		};
	}
}
#endif
