#ifndef TERMINATION_H
#define TERMINATION_H

#include "communication/communication.h"
#include "api/communication/communication.h"
#include "api/synchro/hard/hard_sync_mutex.h"

namespace fpmas::synchro::hard {

	using api::synchro::hard::Color;
	using api::synchro::hard::Tag;
	using api::synchro::hard::Epoch;

	template<template<typename> class TypedMpi>
		class TerminationAlgorithm
			: public fpmas::api::synchro::hard::TerminationAlgorithm {
				typedef fpmas::api::communication::MpiCommunicator comm_t;
				typedef fpmas::api::synchro::hard::Server server_t;
			private:
				comm_t& comm;
				TypedMpi<Color> colorMpi {comm};
				Color color = Color::WHITE;

				void toggleEpoch(server_t& server);

			public:
				TerminationAlgorithm(comm_t& comm) : comm(comm) {}
				const TypedMpi<Color>& getColorMpi() const {return colorMpi;}
				void terminate(server_t& server) override;
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
		void TerminationAlgorithm<TypedMpi>::terminate(server_t& server) {
			FPMAS_LOGD(comm.getRank(), "TERMINATION", "Entering termination algorithm... (epoch : %i)",
					server.getEpoch());
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
							toggleEpoch(server);
							FPMAS_LOGD(comm.getRank(), "TERMINATION", "Termination complete.");
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
				if(this->comm.getRank() > 0 && this->comm.Iprobe(0, Tag::END, &status) > 0) {
					this->comm.recv(&status);
					toggleEpoch(server);
					FPMAS_LOGD(comm.getRank(), "TERMINATION", "End message received, termination complete.");
					return;
				}

				// Handles READ or ACQUIRE requests
				server.handleIncomingRequests();
			}
		}
}
#endif
