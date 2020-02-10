#ifndef SYNC_COMMUNICATION_H
#define SYNC_COMMUNICATION_H

#include "communication.h"
#include "resource_container.h"
#include "readers_writers.h"

namespace FPMAS::communication {
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
		 * An MpiCommunicator extension that implements the
		 * Dijkstra-Feijen-Gasteren termination algorithm.
		 *
		 * The user can invoke the terminate() function to synchronize the
		 * communicators.
		 *
		 */
		class SyncMpiCommunicator : public MpiCommunicator {
			friend ReadersWriters;
			private:
				State state = State::ACTIVE;
				Color color = Color::WHITE;

				ResourceContainer& resourceContainer;
				ResourceManager resourceManager;
				

				void send(std::string, int, Tag);
				void waitSendRequest(MPI_Request*);

				void handleRead(int, unsigned long);
				void respondToRead(int, unsigned long);
				void handleAcquire(int, unsigned long);
				void respondToAcquire(int, unsigned long);
				void handleGiveBack(std::string);

				void waitForReading(unsigned long);
				void waitForAcquire(unsigned long);

			public:
				SyncMpiCommunicator(ResourceContainer&);
				SyncMpiCommunicator(ResourceContainer&, std::initializer_list<int>);

				State getState() const;

				void handleIncomingRequests();

				std::string read(unsigned long, int);

				std::string acquire(unsigned long, int);
				void giveBack(unsigned long, int);

				void terminate();

		};

}
#endif
