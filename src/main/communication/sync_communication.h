#ifndef SYNC_COMMUNICATION_H
#define SYNC_COMMUNICATION_H

#include "communication.h"
#include "resource_container.h"
#include "readers_writers.h"

namespace FPMAS::communication {

	/**
	 * Tags used to smartly manage MPI communications.
	 */
		enum Tag : int {
			READ = 0x00,
			READ_RESPONSE = 0x01,
			ACQUIRE = 0x02,
			ACQUIRE_RESPONSE = 0x03,
			ACQUIRE_GIVE_BACK = 0x04,
			TOKEN = 0x05,
			END = 0x06
		};

		/**
		 * Used to smartly manage message reception to prevent message mixing
		 * accross several time steps.
		 */
		enum Epoch : int {
			EVEN = 0x00,
			ODD = 0x10
		};

		/**
		 * Used by the termination algorithm.
		 */
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
				Epoch epoch = EVEN;
				Color color = WHITE;

				ResourceContainer& resourceContainer;
				ResourceManager resourceManager;
				

				void send(std::string, int, Tag);
				void waitSendRequest(MPI_Request*);

				void handleRead(int, IdType);
				void respondToRead(int, IdType);
				void handleAcquire(int, IdType);
				void respondToAcquire(int, IdType);
				void handleGiveBack(std::string);

				void waitForReading(IdType);
				void waitForAcquire(IdType);

			public:
				SyncMpiCommunicator(ResourceContainer&);
				SyncMpiCommunicator(ResourceContainer&, std::initializer_list<int>);

				void handleIncomingRequests();

				std::string read(IdType, int);

				std::string acquire(IdType, int);
				void giveBack(IdType, int);

				void terminate();

		};

}
#endif
