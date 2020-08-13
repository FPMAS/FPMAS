#ifndef TERMINATION_H
#define TERMINATION_H

#include "fpmas/communication/communication.h"
#include "fpmas/api/communication/communication.h"
#include "fpmas/synchro/hard/api/hard_sync_mutex.h"
#include "fpmas/utils/log.h"

namespace fpmas { namespace synchro { namespace hard {

	using api::Color;
	using api::Tag;
	using api::Epoch;

	/**
	 * api::TerminationAlgorithm implementation.
	 *
	 * More precisely, this class implements the [Dijkstra-Feijen-Gasteren
	 * algorithm](https://link.springer.com/chapter/10.1007%2F978-3-642-82921-5_13)
	 * to establish the termination of the provided api::Server.
	 */
	class TerminationAlgorithm
		: public api::TerminationAlgorithm {
			private:
				fpmas::api::communication::MpiCommunicator& comm;
				fpmas::api::communication::TypedMpi<Color>& color_mpi;
				Color color = Color::WHITE;

				void toggleEpoch(api::Server& server);

			public:
				/**
				 * TerminationAlgorithm constructor.
				 *
				 * @param comm MPI communicator used to exchange termination
				 * messages
				 * @param color_mpi Typed MPI instance used to transmit Color
				 */
				TerminationAlgorithm(
						fpmas::api::communication::MpiCommunicator& comm,
						fpmas::api::communication::TypedMpi<Color>& color_mpi
						) : comm(comm), color_mpi(color_mpi) {}
				void terminate(api::Server& server) override;
		};
}}}
#endif
