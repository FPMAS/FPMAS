#ifndef TERMINATION_H
#define TERMINATION_H

#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {

	using api::communication::Color;
	using api::graph::parallel::synchro::hard::Tag;
	using api::graph::parallel::synchro::hard::Epoch;

	template<typename T>
		class TerminationAlgorithm
			: public FPMAS::api::graph::parallel::synchro::hard::TerminationAlgorithm<T> {
				typedef FPMAS::api::communication::MpiCommunicator comm_t;
				typedef FPMAS::api::graph::parallel::synchro::hard::MutexServer<T> mutex_server;
			private:
				comm_t& comm;
				Color color;

				void toggleEpoch(mutex_server& server);

			public:
				TerminationAlgorithm(comm_t& comm) : comm(comm) {}
				void terminate(mutex_server& mutexServer) override;
		};

	template<typename T>
		void TerminationAlgorithm<T>::toggleEpoch(mutex_server& server) {
			switch(server.getEpoch()) {
				case Epoch::EVEN:
					server.setEpoch(Epoch::ODD);
					break;
				case Epoch::ODD:
					server.setEpoch(Epoch::EVEN);
			}
		}

	template<typename T>
		void TerminationAlgorithm<T>::terminate(mutex_server& mutexServer) {
			Color token;

			if(this->comm.getRank() == 0) {
				this->color = Color::WHITE;
				token = Color::WHITE;
				this->comm.send(token, this->comm.getSize() - 1, Tag::TOKEN);
			}

			int sup_rank = (this->comm.getRank() + 1) % this->comm.getSize();
			MPI_Status status;
			while(true) {
				// Check for TOKEN
				if(this->comm.Iprobe(sup_rank, Tag::TOKEN, &status) > 0) {
					this->comm.recv(&status, token);
					if(this->comm.getRank() == 0) {
						if(token == Color::WHITE && this->color == Color::WHITE) {
							for (int i = 1; i < this->comm.getSize(); ++i) {
								this->comm.sendEnd(i);
							}
							toggleEpoch(mutexServer);
							return;
						} else {
							this->color = Color::WHITE;
							token = Color::WHITE;
							this->comm.send(token, this->comm.getSize() - 1, Tag::TOKEN);
						}
					}
					else {
						if(this->color == Color::BLACK) {
							token = Color::BLACK;
						}
						this->comm.send(token, this->comm.getRank() - 1, Tag::TOKEN);
						this->color = Color::WHITE;
					}
				}

				// Check for END
				if(this->comm.getRank() > 0 && this->comm.Iprobe(MPI_ANY_SOURCE, Tag::END, &status) > 0) {
					int end;
					this->comm.recv(&status);
					toggleEpoch(mutexServer);
					return;
				}

				// Handles READ or ACQUIRE requests
				mutexServer.handleIncomingRequests();
			}
		}

}
#endif
