#ifndef TERMINATION_H
#define TERMINATION_H

#include "communication/communication.h"
#include "api/communication/communication.h"
#include "api/graph/parallel/synchro/hard/hard_sync_mutex.h"

namespace FPMAS::graph::parallel::synchro::hard {

	using api::graph::parallel::synchro::hard::Color;
	using api::graph::parallel::synchro::hard::Tag;
	using api::graph::parallel::synchro::hard::Epoch;

	template<template<typename> class TypedMpi>
		class TerminationAlgorithm
			: public FPMAS::api::graph::parallel::synchro::hard::TerminationAlgorithm {
				typedef FPMAS::api::communication::MpiCommunicator comm_t;
				typedef FPMAS::api::graph::parallel::synchro::hard::Server server_t;
			private:
				comm_t& comm;
				TypedMpi<Color> colorMpi {comm};
				Color color;

				void toggleEpoch(server_t& server);

			public:
				TerminationAlgorithm(comm_t& comm) : comm(comm) {}
				const TypedMpi<Color>& getColorMpi() const {return colorMpi;}
				void terminate(server_t& mutexServer) override;
		};

	template<template<typename> class TypedMpi>
		void TerminationAlgorithm<TypedMpi>::toggleEpoch(server_t& server) {
			switch(server.getEpoch()) {
				case Epoch::EVEN:
					server.setEpoch(Epoch::ODD);
					break;
				case Epoch::ODD:
					server.setEpoch(Epoch::EVEN);
			}
		}

	template<template<typename> class TypedMpi>
		void TerminationAlgorithm<TypedMpi>::terminate(server_t& mutexServer) {
			Color token;

			if(this->comm.getRank() == 0) {
				this->color = Color::WHITE;
				token = Color::WHITE;
				this->colorMpi.send(token, this->comm.getSize() - 1, Tag::TOKEN);
			}

			int sup_rank = (this->comm.getRank() + 1) % this->comm.getSize();
			MPI_Status status;
			while(true) {
				// Check for TOKEN
				if(this->comm.Iprobe(sup_rank, Tag::TOKEN, &status) > 0) {
					token = this->colorMpi.recv(&status);
					if(this->comm.getRank() == 0) {
						if(token == Color::WHITE && this->color == Color::WHITE) {
							for (int i = 1; i < this->comm.getSize(); ++i) {
								this->comm.send(i, Tag::END);
							}
							toggleEpoch(mutexServer);
							return;
						} else {
							this->color = Color::WHITE;
							token = Color::WHITE;
							this->colorMpi.send(token, this->comm.getSize() - 1, Tag::TOKEN);
						}
					}
					else {
						if(this->color == Color::BLACK) {
							token = Color::BLACK;
						}
						this->colorMpi.send(token, this->comm.getRank() - 1, Tag::TOKEN);
						this->color = Color::WHITE;
					}
				}

				// Check for END
				if(this->comm.getRank() > 0 && this->comm.Iprobe(MPI_ANY_SOURCE, Tag::END, &status) > 0) {
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
