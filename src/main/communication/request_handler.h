#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include "api/communication/request_handler.h"
#include "api/communication/resource_handler.h"

#include "communication.h"
#include "readers_writers.h"

namespace FPMAS::communication {

	using api::communication::Epoch;
	using api::communication::Tag;
	using api::communication::Color;
	using api::communication::ResourceHandler;

	/**
	 * An MpiCommunicator extension that implements the
	 * Dijkstra-Feijen-Gasteren termination algorithm.
	 *
	 * The user can invoke the terminate() function to synchronize the
	 * communicators.
	 *
	 */
	class RequestHandler
		: public api::communication::RequestHandler {
			typedef api::communication::MpiCommunicator mpi_communicator;
			friend ReadersWriters;

			private:
			mpi_communicator& comm;
			MpiDistributedId distributedIdBuffer;

			Epoch epoch = Epoch::EVEN;
			Color color = Color::WHITE;

			ResourceHandler& resourceHandler;
			ResourceManager resourceManager;

			void waitSendRequest(MPI_Request*);

			void handleRead(int, DistributedId);
			void respondToRead(int, DistributedId);
			void handleAcquire(int, DistributedId);
			void respondToAcquire(int, DistributedId);
			void handleGiveBack(std::string);

			const mpi_communicator& getMpiComm() override;

			void waitForReading(DistributedId);
			void waitForAcquire(DistributedId);


			public:
			RequestHandler(mpi_communicator&, ResourceHandler&);

			void handleIncomingRequests();

			std::string read(DistributedId, int) override;

			std::string acquire(DistributedId, int) override;
			void giveBack(DistributedId, int) override;

			void terminate() override;

		};
}
#endif
